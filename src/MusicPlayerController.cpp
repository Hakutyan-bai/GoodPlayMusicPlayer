#include "MusicPlayerController.h"

#include "ImportScanWorker.h"
#include "TrackImportUtils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QImage>
#include <QMetaType>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>

#include <algorithm>

namespace {

constexpr qint64 SpectrumFrameIntervalMs = 33;

} // namespace

MusicPlayerController::MusicPlayerController(QObject *parent)
    : QObject(parent)
    , m_trackModel(this)
{
    qRegisterMetaType<TrackListModel::TrackItem>("TrackListModel::TrackItem");
    qRegisterMetaType<QList<TrackListModel::TrackItem>>("QList<TrackListModel::TrackItem>");

    m_audioOutput.setVolume(0.78);
    m_player.setAudioOutput(&m_audioOutput);
    m_player.setAudioBufferOutput(&m_audioBufferOutput);
    m_spectrumValues = toVariantList(QVector<qreal>(SpectrumAnalyzer::BandCount, 0.0));
    m_spectrumFrameTimer.start();

    m_importWorker = new ImportScanWorker();
    m_importWorker->moveToThread(&m_importThread);
    connect(&m_importThread, &QThread::finished, m_importWorker, &QObject::deleteLater);
    connect(this,
            &MusicPlayerController::importFilesRequested,
            m_importWorker,
            &ImportScanWorker::importFiles,
            Qt::QueuedConnection);
    connect(this,
            &MusicPlayerController::importFolderRequested,
            m_importWorker,
            &ImportScanWorker::importFolder,
            Qt::QueuedConnection);
    connect(m_importWorker,
            &ImportScanWorker::tracksReady,
            this,
            &MusicPlayerController::handleImportedTracks,
            Qt::QueuedConnection);
    connect(m_importWorker,
            &ImportScanWorker::trackMetadataResolved,
            this,
            &MusicPlayerController::handleTrackMetadataResolved,
            Qt::QueuedConnection);
    connect(m_importWorker,
            &ImportScanWorker::importFinished,
            this,
            &MusicPlayerController::handleImportFinished,
            Qt::QueuedConnection);
    m_importThread.start();

    connect(&m_player, &QMediaPlayer::positionChanged, this, &MusicPlayerController::positionChanged);
    connect(&m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        if (m_currentIndex >= 0) {
            m_trackModel.setTrackDuration(m_currentIndex, duration);
        }
        emit durationChanged(duration);
    });
    connect(&m_player, &QMediaPlayer::playingChanged, this, &MusicPlayerController::playingChanged);
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            const int total = m_trackModel.rowCount();
            if (total > 0) {
                playAt((m_currentIndex + 1 + total) % total);
            }
        }
    });
    connect(&m_player,
            &QMediaPlayer::errorOccurred,
            this,
            [this](QMediaPlayer::Error error, const QString &errorString) {
                Q_UNUSED(error)
                setErrorMessage(errorString.isEmpty() ? tr("无法播放当前音频文件。") : errorString);
            });
    connect(&m_audioOutput, &QAudioOutput::volumeChanged, this, [this]() {
        emit volumeChanged();
    });
    connect(&m_audioBufferOutput,
            &QAudioBufferOutput::audioBufferReceived,
            this,
            [this](const QAudioBuffer &buffer) {
                if (m_spectrumFrameTimer.isValid()
                    && m_spectrumFrameTimer.elapsed() < SpectrumFrameIntervalMs) {
                    return;
                }

                m_spectrumFrameTimer.restart();
                m_spectrumValues = toVariantList(m_spectrumAnalyzer.analyze(buffer));
                emit spectrumValuesChanged();
            });
    connect(&m_player, &QMediaPlayer::metaDataChanged, this, &MusicPlayerController::updateCoverArt);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        stopPlaybackImmediately();
        if (m_importWorker) {
            m_importWorker->requestCancel();
        }
    });
}

MusicPlayerController::~MusicPlayerController()
{
    stopPlaybackImmediately();
    if (m_importWorker) {
        m_importWorker->requestCancel();
    }
    m_importThread.quit();
    m_importThread.wait();
}

TrackListModel *MusicPlayerController::trackModel()
{
    return &m_trackModel;
}

int MusicPlayerController::currentIndex() const
{
    return m_currentIndex;
}

QString MusicPlayerController::currentTitle() const
{
    if (m_currentIndex < 0) {
        return tr("还没有加载歌曲");
    }

    return m_trackModel.at(m_currentIndex).title;
}

QString MusicPlayerController::currentSubtitle() const
{
    if (m_currentIndex < 0) {
        return tr("先打开文件或文件夹，播放器就会开始工作");
    }

    return m_trackModel.at(m_currentIndex).subtitle;
}

qint64 MusicPlayerController::position() const
{
    return m_player.position();
}

qint64 MusicPlayerController::duration() const
{
    return m_player.duration();
}

bool MusicPlayerController::playing() const
{
    return m_player.isPlaying();
}

qreal MusicPlayerController::volume() const
{
    return m_audioOutput.volume();
}

QUrl MusicPlayerController::coverArtUrl() const
{
    return m_coverArtUrl;
}

QString MusicPlayerController::errorMessage() const
{
    return m_errorMessage;
}

QVariantList MusicPlayerController::spectrumValues() const
{
    return m_spectrumValues;
}

void MusicPlayerController::openFiles()
{
    const QString startDirectory =
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    const QStringList files = QFileDialog::getOpenFileNames(
        nullptr,
        tr("打开音频文件"),
        startDirectory,
        audioDialogFilter());

    beginFileImport(files);
}

void MusicPlayerController::openFolder()
{
    const QString startDirectory =
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    const QString folder = QFileDialog::getExistingDirectory(
        nullptr,
        tr("打开音乐文件夹"),
        startDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    beginFolderImport(folder);
}

void MusicPlayerController::playPause()
{
    if (m_currentIndex < 0 && m_trackModel.rowCount() > 0) {
        playAt(0);
        return;
    }

    if (m_player.isPlaying()) {
        m_player.pause();
    } else {
        m_player.play();
    }
}

void MusicPlayerController::playAt(int index)
{
    if (index < 0 || index >= m_trackModel.rowCount()) {
        return;
    }

    if (m_currentIndex == index && m_player.source() == m_trackModel.at(index).source) {
        m_player.play();
        return;
    }

    m_currentIndex = index;
    const TrackListModel::TrackItem track = m_trackModel.at(index);

    setErrorMessage({});
    m_player.setSource(track.source);
    if (track.coverArtUrl.isEmpty() || TrackImportUtils::isDefaultCoverArtUrl(track.coverArtUrl)) {
        setCoverArtUrl(createGeneratedCoverArt(track.source));
    } else {
        setCoverArtUrl(track.coverArtUrl);
    }
    m_player.play();

    emit currentIndexChanged();
    emit currentTrackChanged();
}

void MusicPlayerController::next()
{
    const int total = m_trackModel.rowCount();
    if (total <= 0) {
        return;
    }

    const int nextIndex = (m_currentIndex + 1 + total) % total;
    playAt(nextIndex);
}

void MusicPlayerController::previous()
{
    const int total = m_trackModel.rowCount();
    if (total <= 0) {
        return;
    }

    if (m_player.position() > 4000) {
        m_player.setPosition(0);
        return;
    }

    const int previousIndex = (m_currentIndex - 1 + total) % total;
    playAt(previousIndex);
}

void MusicPlayerController::setPosition(qint64 position)
{
    m_player.setPosition(std::max<qint64>(0, position));
}

void MusicPlayerController::setVolume(qreal volume)
{
    m_audioOutput.setVolume(std::clamp(volume, 0.0, 1.0));
}

QString MusicPlayerController::formatTime(qint64 milliseconds) const
{
    if (milliseconds <= 0) {
        return QStringLiteral("00:00");
    }

    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar(u'0'))
            .arg(seconds, 2, 10, QChar(u'0'));
    }

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QChar(u'0'))
        .arg(seconds, 2, 10, QChar(u'0'));
}

void MusicPlayerController::beginFileImport(const QStringList &filePaths)
{
    if (filePaths.isEmpty()) {
        return;
    }

    const int requestId = ++m_nextImportRequestId;
    m_importAddedCounts.insert(requestId, 0);
    if (m_trackModel.rowCount() == 0 && m_currentIndex < 0) {
        m_autoplayRequestIds.insert(requestId);
    }

    emit importFilesRequested(requestId, filePaths);
}

void MusicPlayerController::beginFolderImport(const QString &folderPath)
{
    if (folderPath.isEmpty()) {
        return;
    }

    const int requestId = ++m_nextImportRequestId;
    m_importAddedCounts.insert(requestId, 0);
    if (m_trackModel.rowCount() == 0 && m_currentIndex < 0) {
        m_autoplayRequestIds.insert(requestId);
    }

    emit importFolderRequested(requestId, folderPath, audioNameFilters());
}

void MusicPlayerController::handleImportedTracks(int requestId, const QList<TrackListModel::TrackItem> &tracks)
{
    QList<TrackListModel::TrackItem> filteredTracks;
    filteredTracks.reserve(tracks.size());

    for (const TrackListModel::TrackItem &track : tracks) {
        const QString key = sourceKey(track.source);
        if (key.isEmpty() || m_knownSourceKeys.contains(key)) {
            continue;
        }

        m_knownSourceKeys.insert(key);
        TrackListModel::TrackItem preparedTrack = track;
        if (preparedTrack.coverArtUrl.isEmpty()) {
            preparedTrack.coverArtUrl = TrackImportUtils::defaultCoverArtUrl();
        }
        filteredTracks.append(preparedTrack);
    }

    if (filteredTracks.isEmpty()) {
        return;
    }

    const int firstInsertedIndex = m_trackModel.rowCount();
    m_trackModel.addTracks(filteredTracks);
    for (int i = 0; i < filteredTracks.size(); ++i) {
        m_sourceIndexMap.insert(sourceKey(filteredTracks.at(i).source), firstInsertedIndex + i);
    }
    m_importAddedCounts[requestId] = m_importAddedCounts.value(requestId) + filteredTracks.count();
    setErrorMessage({});

    if (m_autoplayRequestIds.contains(requestId) && m_currentIndex < 0) {
        m_autoplayRequestIds.remove(requestId);
        playAt(firstInsertedIndex);
    }
}

void MusicPlayerController::handleTrackMetadataResolved(int requestId,
                                                        const QUrl &source,
                                                        qint64 duration,
                                                        const QUrl &coverArtUrl)
{
    Q_UNUSED(requestId)

    const QString key = sourceKey(source);
    if (key.isEmpty() || !m_sourceIndexMap.contains(key)) {
        return;
    }

    const int index = m_sourceIndexMap.value(key);
    if (duration > 0) {
        m_trackModel.setTrackDuration(index, duration);
    }

    if (!coverArtUrl.isEmpty()) {
        m_trackModel.setTrackCoverArt(index, coverArtUrl);
        if (index == m_currentIndex) {
            setCoverArtUrl(coverArtUrl);
        }
    }
}

void MusicPlayerController::handleImportFinished(int requestId, int emittedCount, bool hadInput)
{
    Q_UNUSED(emittedCount)

    const int addedCount = m_importAddedCounts.take(requestId);
    m_autoplayRequestIds.remove(requestId);

    if (addedCount == 0 && hadInput) {
        setErrorMessage(tr("没有找到可导入的新音频文件。"));
    }
}

QString MusicPlayerController::sourceKey(const QUrl &source) const
{
    if (source.isLocalFile()) {
        return source.toLocalFile();
    }

    return source.toString();
}

QStringList MusicPlayerController::audioNameFilters() const
{
    return {
        QStringLiteral("*.mp3"),
        QStringLiteral("*.wav"),
        QStringLiteral("*.flac"),
        QStringLiteral("*.m4a"),
        QStringLiteral("*.aac"),
        QStringLiteral("*.ogg"),
        QStringLiteral("*.opus"),
        QStringLiteral("*.wma")
    };
}

QString MusicPlayerController::audioDialogFilter() const
{
    return tr("音频文件 (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.opus *.wma)");
}

QVariantList MusicPlayerController::toVariantList(const QVector<qreal> &values) const
{
    QVariantList list;
    list.reserve(values.size());

    for (qreal value : values) {
        list.append(value);
    }

    return list;
}

void MusicPlayerController::setErrorMessage(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }

    m_errorMessage = message;
    emit errorMessageChanged();
}

void MusicPlayerController::stopPlaybackImmediately()
{
    m_player.stop();
    m_player.setSource(QUrl());
    m_player.setPosition(0);
    m_audioOutput.setVolume(0.0);
}

void MusicPlayerController::updateCoverArt()
{
    if (m_currentIndex < 0) {
        setCoverArtUrl({});
        return;
    }

    const TrackListModel::TrackItem track = m_trackModel.at(m_currentIndex);
    const QUrl embeddedCoverUrl = TrackImportUtils::coverArtUrlFromMetaData(track.source, m_player.metaData());
    const QUrl coverUrl = embeddedCoverUrl.isEmpty()
                              ? ((track.coverArtUrl.isEmpty()
                                  || TrackImportUtils::isDefaultCoverArtUrl(track.coverArtUrl))
                                     ? createGeneratedCoverArt(track.source)
                                     : track.coverArtUrl)
                              : embeddedCoverUrl;

    m_trackModel.setTrackCoverArt(m_currentIndex, coverUrl);
    setCoverArtUrl(coverUrl);
}

void MusicPlayerController::setCoverArtUrl(const QUrl &url)
{
    if (m_coverArtUrl == url) {
        return;
    }

    m_coverArtUrl = url;
    emit coverArtUrlChanged();
}

QUrl MusicPlayerController::createGeneratedCoverArt(const QUrl &source) const
{
    if (source.isEmpty()) {
        return {};
    }

    const QByteArray digest = QCryptographicHash::hash(source.toString().toUtf8(), QCryptographicHash::Sha1);

    auto colorFromDigest = [&digest](int offset) {
        return QColor::fromHsl(
            static_cast<uchar>(digest.at(offset)),
            170 + (static_cast<uchar>(digest.at(offset + 1)) / 4),
            88 + (static_cast<uchar>(digest.at(offset + 2)) / 5));
    };

    const QColor colorA = colorFromDigest(0);
    const QColor colorB = colorFromDigest(3);
    const QColor colorC = colorFromDigest(6);

    QImage image(720, 720, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, image.width(), image.height());
    background.setColorAt(0.0, colorA.darker(185));
    background.setColorAt(0.55, colorB.darker(165));
    background.setColorAt(1.0, colorC.darker(220));
    painter.fillRect(image.rect(), background);

    QRadialGradient halo(QPointF(image.width() * 0.72, image.height() * 0.28), image.width() * 0.55);
    halo.setColorAt(0.0, QColor(colorC.red(), colorC.green(), colorC.blue(), 150));
    halo.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(image.rect(), halo);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 18));
    painter.drawEllipse(QPointF(image.width() * 0.22, image.height() * 0.24), 150, 150);
    painter.setBrush(QColor(255, 255, 255, 12));
    painter.drawEllipse(QPointF(image.width() * 0.80, image.height() * 0.76), 190, 190);

    QPainterPath discPath;
    discPath.addEllipse(QRectF(96, 96, image.width() - 192, image.height() - 192));
    painter.fillPath(discPath, QColor(7, 17, 27, 95));
    painter.setPen(QPen(QColor(255, 255, 255, 34), 3));
    painter.drawEllipse(QRectF(126, 126, image.width() - 252, image.height() - 252));
    painter.drawEllipse(QRectF(188, 188, image.width() - 376, image.height() - 376));

    painter.setPen(QPen(QColor(255, 255, 255, 72), 14));
    painter.drawArc(QRectF(180, 180, image.width() - 360, image.height() - 360), 24 * 16, 110 * 16);
    painter.setPen(QPen(QColor(colorC.red(), colorC.green(), colorC.blue(), 160), 10));
    painter.drawArc(QRectF(220, 220, image.width() - 440, image.height() - 440), 210 * 16, 96 * 16);
    painter.setBrush(QColor(255, 255, 255, 210));
    painter.drawEllipse(QPointF(image.width() * 0.5, image.height() * 0.5), 18, 18);

    painter.end();
    return TrackImportUtils::saveCoverArtImage(source, image, QStringLiteral("generated"));
}
