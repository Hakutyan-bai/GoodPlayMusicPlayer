#include "MusicPlayerController.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMediaMetaData>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>
#include <QDateTime>
#include <QCryptographicHash>

#include <algorithm>

namespace {

QString composeSubtitle(const QFileInfo &info)
{
    const QString folderName = info.dir().dirName();
    const QString extension = info.suffix().toUpper();

    if (folderName.isEmpty()) {
        return extension;
    }

    if (extension.isEmpty()) {
        return folderName;
    }

    return QStringLiteral("%1  ·  %2").arg(folderName, extension);
}

QImage circularCoverImage(const QImage &sourceImage, int size = 720)
{
    if (sourceImage.isNull()) {
        return {};
    }

    const QImage scaled =
        sourceImage.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QPoint topLeft((scaled.width() - size) / 2, (scaled.height() - size) / 2);

    QImage target(size, size, QImage::Format_ARGB32_Premultiplied);
    target.fill(Qt::transparent);

    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath clipPath;
    clipPath.addEllipse(QRectF(2, 2, size - 4, size - 4));
    painter.setClipPath(clipPath);
    painter.drawImage(QRect(0, 0, size, size), scaled, QRect(topLeft, QSize(size, size)));
    painter.setClipping(false);

    painter.setPen(QPen(QColor(255, 255, 255, 48), 4));
    painter.drawEllipse(QRectF(4, 4, size - 8, size - 8));
    painter.end();

    return target;
}

} // namespace

MusicPlayerController::MusicPlayerController(QObject *parent)
    : QObject(parent)
    , m_trackModel(this)
{
    m_audioOutput.setVolume(0.78);
    m_player.setAudioOutput(&m_audioOutput);
    m_player.setAudioBufferOutput(&m_audioBufferOutput);
    m_spectrumValues = toVariantList(QVector<qreal>(SpectrumAnalyzer::BandCount, 0.0));

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
            if (m_currentIndex >= 0 && m_currentIndex < (m_trackModel.rowCount() - 1)) {
                playAt(m_currentIndex + 1);
            } else {
                m_player.stop();
                m_player.setPosition(0);
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
                const QVariantList analyzedValues = toVariantList(m_spectrumAnalyzer.analyze(buffer));
                if (analyzedValues != m_spectrumValues) {
                    m_spectrumValues = analyzedValues;
                    emit spectrumValuesChanged();
                }
            });
    connect(&m_player, &QMediaPlayer::metaDataChanged, this, &MusicPlayerController::updateCoverArt);
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

    appendSources(files);
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

    if (folder.isEmpty()) {
        return;
    }

    QStringList files;
    QDirIterator iterator(folder, audioNameFilters(), QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        files.append(iterator.next());
    }

    appendSources(files);
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
    setCoverArtUrl(createGeneratedCoverArt());
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

void MusicPlayerController::appendSources(const QStringList &filePaths)
{
    QList<TrackListModel::TrackItem> tracksToAdd;

    for (const QString &path : filePaths) {
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile()) {
            continue;
        }

        const QUrl source = QUrl::fromLocalFile(info.absoluteFilePath());
        if (m_trackModel.containsSource(source)) {
            continue;
        }

        TrackListModel::TrackItem track;
        track.title = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
        track.subtitle = composeSubtitle(info);
        track.path = info.absoluteFilePath();
        track.source = source;
        tracksToAdd.append(track);
    }

    if (tracksToAdd.isEmpty()) {
        if (!filePaths.isEmpty()) {
            setErrorMessage(tr("没有找到可导入的新音频文件。"));
        }
        return;
    }

    const bool shouldAutoplay = (m_trackModel.rowCount() == 0);
    m_trackModel.addTracks(tracksToAdd);
    setErrorMessage({});

    if (shouldAutoplay) {
        playAt(0);
    }
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

void MusicPlayerController::updateCoverArt()
{
    if (m_currentIndex < 0) {
        setCoverArtUrl({});
        return;
    }

    const QMediaMetaData metaData = m_player.metaData();

    const QVariant coverVariant = metaData.value(QMediaMetaData::CoverArtImage);
    if (coverVariant.canConvert<QImage>()) {
        const QImage coverImage = qvariant_cast<QImage>(coverVariant);
        if (!coverImage.isNull()) {
            setCoverArtUrl(saveCoverArtImage(coverImage, QStringLiteral("cover")));
            return;
        }
    }

    const QVariant thumbnailVariant = metaData.value(QMediaMetaData::ThumbnailImage);
    if (thumbnailVariant.canConvert<QImage>()) {
        const QImage thumbnailImage = qvariant_cast<QImage>(thumbnailVariant);
        if (!thumbnailImage.isNull()) {
            setCoverArtUrl(saveCoverArtImage(thumbnailImage, QStringLiteral("thumb")));
            return;
        }
    }

    setCoverArtUrl(createGeneratedCoverArt());
}

void MusicPlayerController::setCoverArtUrl(const QUrl &url)
{
    if (m_coverArtUrl == url) {
        return;
    }

    m_coverArtUrl = url;
    emit coverArtUrlChanged();
}

QUrl MusicPlayerController::createGeneratedCoverArt() const
{
    if (m_currentIndex < 0) {
        return {};
    }

    const TrackListModel::TrackItem track = m_trackModel.at(m_currentIndex);
    const QByteArray digest =
        QCryptographicHash::hash(track.source.toString().toUtf8(), QCryptographicHash::Sha1);

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
    return saveCoverArtImage(image, QStringLiteral("generated"));
}

QUrl MusicPlayerController::saveCoverArtImage(const QImage &image, const QString &suffix) const
{
    if (m_currentIndex < 0 || image.isNull()) {
        return {};
    }

    const TrackListModel::TrackItem track = m_trackModel.at(m_currentIndex);
    const QString cacheDirPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/GoodPlayMusicPlayer");
    QDir cacheDir(cacheDirPath);
    if (!cacheDir.exists()) {
        cacheDir.mkpath(QStringLiteral("."));
    }

    const QByteArray digest =
        QCryptographicHash::hash(track.source.toString().toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString filePath = cacheDir.filePath(QStringLiteral("%1_%2.png").arg(QString::fromLatin1(digest), suffix));

    const QImage circularImage = circularCoverImage(image);
    circularImage.save(filePath, "PNG");

    QUrl url = QUrl::fromLocalFile(filePath);
    url.setQuery(QStringLiteral("v=%1").arg(QDateTime::currentMSecsSinceEpoch()));
    return url;
}
