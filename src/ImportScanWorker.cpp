#include "ImportScanWorker.h"

#include "TrackImportUtils.h"

#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QSet>
#include <QTimer>

namespace {

constexpr int ImportBatchSize = 8;
constexpr int MetadataTimeoutMs = 1200;
constexpr int MetadataSettleDelayMs = 180;

} // namespace

ImportScanWorker::ImportScanWorker(QObject *parent)
    : QObject(parent)
{
}

void ImportScanWorker::requestCancel()
{
    m_cancelRequested.store(true);
}

void ImportScanWorker::importFiles(int requestId, const QStringList &filePaths)
{
    m_cancelRequested.store(false);
    processFileIterator(requestId, filePaths, !filePaths.isEmpty());
}

void ImportScanWorker::importFolder(int requestId, const QString &folderPath, const QStringList &nameFilters)
{
    m_cancelRequested.store(false);
    if (folderPath.isEmpty()) {
        emit importFinished(requestId, 0, false);
        return;
    }

    QStringList filePaths;
    QDirIterator iterator(folderPath, nameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        filePaths.append(iterator.next());
    }

    processFileIterator(requestId, filePaths, true);
}

void ImportScanWorker::ensureMetadataPlayer()
{
    if (!m_metadataPlayer) {
        m_metadataPlayer = new QMediaPlayer(this);
    }
}

void ImportScanWorker::processFileIterator(int requestId, const QStringList &filePaths, bool hadInput)
{
    QList<TrackListModel::TrackItem> batch;
    batch.reserve(ImportBatchSize);
    QList<TrackListModel::TrackItem> discoveredTracks;

    QSet<QString> seenPaths;
    int emittedCount = 0;

    for (const QString &path : filePaths) {
        if (m_cancelRequested.load()) {
            break;
        }

        const QString normalizedPath = QFileInfo(path).canonicalFilePath();
        const QString dedupePath = normalizedPath.isEmpty() ? QFileInfo(path).absoluteFilePath() : normalizedPath;
        if (dedupePath.isEmpty() || seenPaths.contains(dedupePath)) {
            continue;
        }

        seenPaths.insert(dedupePath);
        const TrackListModel::TrackItem track = createTrackSkeleton(dedupePath);
        if (track.source.isEmpty()) {
            continue;
        }

        discoveredTracks.append(track);
        batch.append(track);
        ++emittedCount;

        if (batch.size() >= ImportBatchSize) {
            emit tracksReady(requestId, batch);
            batch.clear();
        }
    }

    if (!batch.isEmpty()) {
        emit tracksReady(requestId, batch);
    }

    for (const TrackListModel::TrackItem &track : discoveredTracks) {
        if (m_cancelRequested.load()) {
            break;
        }

        resolveTrackMetadata(requestId, track);
    }

    emit importFinished(requestId, emittedCount, hadInput);
}

TrackListModel::TrackItem ImportScanWorker::createTrackSkeleton(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    TrackListModel::TrackItem track;
    track.title = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
    track.subtitle = TrackImportUtils::composeSubtitle(info);
    track.path = info.absoluteFilePath();
    track.source = QUrl::fromLocalFile(info.absoluteFilePath());
    track.coverArtUrl = TrackImportUtils::defaultCoverArtUrl();

    return track;
}

void ImportScanWorker::resolveTrackMetadata(int requestId, const TrackListModel::TrackItem &track)
{
    if (track.source.isEmpty()) {
        return;
    }

    QMediaMetaData metaData;
    qint64 duration = -1;
    loadTrackMetadata(track.source, &duration, &metaData);
    if (m_cancelRequested.load()) {
        return;
    }

    const QUrl embeddedCoverUrl = TrackImportUtils::coverArtUrlFromMetaData(track.source, metaData);
    emit trackMetadataResolved(requestId, track.source, duration, embeddedCoverUrl);
}

void ImportScanWorker::loadTrackMetadata(const QUrl &source, qint64 *duration, QMediaMetaData *metaData)
{
    if (source.isEmpty()) {
        return;
    }

    ensureMetadataPlayer();
    if (!m_metadataPlayer) {
        return;
    }

    if (m_cancelRequested.load()) {
        return;
    }

    QEventLoop loop;
    QTimer timeoutTimer;
    QTimer settleTimer;
    timeoutTimer.setSingleShot(true);
    settleTimer.setSingleShot(true);

    const auto stopLoop = [&loop]() {
        if (loop.isRunning()) {
            loop.quit();
        }
    };

    QMetaObject::Connection durationConnection;
    QMetaObject::Connection metadataConnection;
    QMetaObject::Connection statusConnection;
    QMetaObject::Connection errorConnection;
    QMetaObject::Connection timeoutConnection;
    QMetaObject::Connection settleConnection;

    bool mediaLoaded = false;

    durationConnection = connect(m_metadataPlayer, &QMediaPlayer::durationChanged, &loop, [duration](qint64 value) {
        if (duration && value > 0) {
            *duration = value;
        }
    });
    metadataConnection = connect(m_metadataPlayer,
                                 &QMediaPlayer::metaDataChanged,
                                 &loop,
                                 [this, metaData, &mediaLoaded, &settleTimer]() {
                                     if (metaData) {
                                         *metaData = m_metadataPlayer->metaData();
                                     }

                                     if (mediaLoaded) {
                                         settleTimer.start(MetadataSettleDelayMs);
                                     }
                                 });
    statusConnection = connect(m_metadataPlayer,
                               &QMediaPlayer::mediaStatusChanged,
                               &loop,
                               [this,
                                duration,
                                metaData,
                                &mediaLoaded,
                                &settleTimer,
                                &stopLoop](QMediaPlayer::MediaStatus status) {
                                   if (status == QMediaPlayer::LoadedMedia
                                       || status == QMediaPlayer::BufferedMedia
                                       || status == QMediaPlayer::EndOfMedia) {
                                       mediaLoaded = true;
                                       if (duration && m_metadataPlayer->duration() > 0) {
                                           *duration = m_metadataPlayer->duration();
                                       }
                                       if (metaData) {
                                           *metaData = m_metadataPlayer->metaData();
                                       }
                                       settleTimer.start(MetadataSettleDelayMs);
                                   } else if (status == QMediaPlayer::InvalidMedia) {
                                       stopLoop();
                                   }
                               });
    errorConnection = connect(m_metadataPlayer,
                              &QMediaPlayer::errorOccurred,
                              &loop,
                              [&stopLoop](QMediaPlayer::Error error, const QString &errorString) {
                                  Q_UNUSED(error)
                                  Q_UNUSED(errorString)
                                  stopLoop();
                              });
    timeoutConnection = connect(&timeoutTimer, &QTimer::timeout, &loop, [&stopLoop]() {
        stopLoop();
    });
    settleConnection = connect(&settleTimer, &QTimer::timeout, &loop, [&stopLoop]() {
        stopLoop();
    });

    m_metadataPlayer->stop();
    m_metadataPlayer->setSource(QUrl());
    m_metadataPlayer->setSource(source);
    timeoutTimer.start(MetadataTimeoutMs);
    loop.exec();

    if (duration && m_metadataPlayer->duration() > 0) {
        *duration = m_metadataPlayer->duration();
    }
    if (metaData) {
        *metaData = m_metadataPlayer->metaData();
    }

    timeoutTimer.stop();
    settleTimer.stop();
    m_metadataPlayer->stop();
    m_metadataPlayer->setSource(QUrl());

    disconnect(durationConnection);
    disconnect(metadataConnection);
    disconnect(statusConnection);
    disconnect(errorConnection);
    disconnect(timeoutConnection);
    disconnect(settleConnection);
}
