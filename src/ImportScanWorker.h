#pragma once

#include "TrackListModel.h"

#include <QMediaMetaData>
#include <QObject>
#include <QStringList>

#include <atomic>

class QMediaPlayer;

class ImportScanWorker : public QObject
{
    Q_OBJECT

public:
    explicit ImportScanWorker(QObject *parent = nullptr);
    void requestCancel();

public slots:
    void importFiles(int requestId, const QStringList &filePaths);
    void importFolder(int requestId, const QString &folderPath, const QStringList &nameFilters);

signals:
    void tracksReady(int requestId, const QList<TrackListModel::TrackItem> &tracks);
    void trackMetadataResolved(int requestId, const QUrl &source, qint64 duration, const QUrl &coverArtUrl);
    void importFinished(int requestId, int emittedCount, bool hadInput);

private:
    void ensureMetadataPlayer();
    void processFileIterator(int requestId, const QStringList &filePaths, bool hadInput);
    TrackListModel::TrackItem createTrackSkeleton(const QString &filePath);
    void resolveTrackMetadata(int requestId, const TrackListModel::TrackItem &track);
    void loadTrackMetadata(const QUrl &source, qint64 *duration, QMediaMetaData *metaData);

    QMediaPlayer *m_metadataPlayer = nullptr;
    std::atomic_bool m_cancelRequested = false;
};
