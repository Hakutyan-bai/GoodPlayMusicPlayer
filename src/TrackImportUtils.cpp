#include "TrackImportUtils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QProcess>
#include <QStandardPaths>
#include <QVariant>

namespace {

QString findFfmpegExecutable()
{
    const QString appDirExecutable =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("ffmpeg.exe"));
    if (QFileInfo::exists(appDirExecutable)) {
        return appDirExecutable;
    }

    return QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
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

namespace TrackImportUtils {

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

QUrl defaultCoverArtUrl()
{
    static const QUrl url(QStringLiteral("qrc:/icons/app_icon.png"));
    return url;
}

bool isDefaultCoverArtUrl(const QUrl &url)
{
    return url == defaultCoverArtUrl();
}

QUrl saveCoverArtImage(const QUrl &source, const QImage &image, const QString &suffix)
{
    if (source.isEmpty() || image.isNull()) {
        return {};
    }

    const QString cacheDirPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/GoodPlayMusicPlayer");
    QDir cacheDir(cacheDirPath);
    if (!cacheDir.exists()) {
        cacheDir.mkpath(QStringLiteral("."));
    }

    const QByteArray digest = QCryptographicHash::hash(source.toString().toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString filePath = cacheDir.filePath(QStringLiteral("%1_%2.png").arg(QString::fromLatin1(digest), suffix));

    const QImage circularImage = circularCoverImage(image);
    circularImage.save(filePath, "PNG");

    QUrl url = QUrl::fromLocalFile(filePath);
    url.setQuery(QStringLiteral("v=%1").arg(QDateTime::currentMSecsSinceEpoch()));
    return url;
}

QUrl coverArtUrlFromMetaData(const QUrl &source, const QMediaMetaData &metaData)
{
    if (source.isEmpty()) {
        return {};
    }

    const QVariant coverVariant = metaData.value(QMediaMetaData::CoverArtImage);
    if (coverVariant.canConvert<QImage>()) {
        const QImage coverImage = qvariant_cast<QImage>(coverVariant);
        if (!coverImage.isNull()) {
            return saveCoverArtImage(source, coverImage, QStringLiteral("cover"));
        }
    }

    const QVariant thumbnailVariant = metaData.value(QMediaMetaData::ThumbnailImage);
    if (thumbnailVariant.canConvert<QImage>()) {
        const QImage thumbnailImage = qvariant_cast<QImage>(thumbnailVariant);
        if (!thumbnailImage.isNull()) {
            return saveCoverArtImage(source, thumbnailImage, QStringLiteral("thumb"));
        }
    }

    return extractCoverArtWithFfmpeg(source);
}

QUrl extractCoverArtWithFfmpeg(const QUrl &source)
{
    if (!source.isLocalFile()) {
        return {};
    }

    const QString ffmpegExecutable = findFfmpegExecutable();
    if (ffmpegExecutable.isEmpty()) {
        return {};
    }

    const QString cacheDirPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/GoodPlayMusicPlayer");
    QDir cacheDir(cacheDirPath);
    if (!cacheDir.exists()) {
        cacheDir.mkpath(QStringLiteral("."));
    }

    const QByteArray digest = QCryptographicHash::hash(source.toString().toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString rawOutputPath =
        cacheDir.filePath(QStringLiteral("%1_ffmpeg_cover.png").arg(QString::fromLatin1(digest)));
    QFile::remove(rawOutputPath);

    QProcess process;
    process.start(ffmpegExecutable,
                  {QStringLiteral("-y"),
                   QStringLiteral("-loglevel"),
                   QStringLiteral("error"),
                   QStringLiteral("-i"),
                   source.toLocalFile(),
                   QStringLiteral("-an"),
                   QStringLiteral("-frames:v"),
                   QStringLiteral("1"),
                   rawOutputPath});

    if (!process.waitForFinished(3000) || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0 || !QFileInfo::exists(rawOutputPath)) {
        QFile::remove(rawOutputPath);
        return {};
    }

    const QImage extractedImage(rawOutputPath);
    QFile::remove(rawOutputPath);
    if (extractedImage.isNull()) {
        return {};
    }

    return saveCoverArtImage(source, extractedImage, QStringLiteral("cover"));
}

} // namespace TrackImportUtils
