#pragma once

#include <QFileInfo>
#include <QMediaMetaData>
#include <QUrl>

class QImage;

namespace TrackImportUtils {

QString composeSubtitle(const QFileInfo &info);
QUrl defaultCoverArtUrl();
bool isDefaultCoverArtUrl(const QUrl &url);
QUrl saveCoverArtImage(const QUrl &source, const QImage &image, const QString &suffix);
QUrl coverArtUrlFromMetaData(const QUrl &source, const QMediaMetaData &metaData);
QUrl extractCoverArtWithFfmpeg(const QUrl &source);

} // namespace TrackImportUtils
