#pragma once

#include <QAbstractListModel>
#include <QUrl>

class TrackListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum TrackRoles {
        TitleRole = Qt::UserRole + 1,
        SubtitleRole,
        DurationRole,
        SourceRole,
        PathRole,
        CoverArtRole
    };
    Q_ENUM(TrackRoles)

    struct TrackItem {
        QString title;
        QString subtitle;
        QString path;
        QUrl source;
        QUrl coverArtUrl;
        qint64 duration = -1;
    };

    explicit TrackListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void addTracks(const QList<TrackItem> &tracks);
    TrackItem at(int index) const;
    bool containsSource(const QUrl &source) const;
    void setTrackDuration(int index, qint64 duration);
    void setTrackCoverArt(int index, const QUrl &coverArtUrl);

signals:
    void countChanged();

private:
    QList<TrackItem> m_tracks;
};

Q_DECLARE_METATYPE(TrackListModel::TrackItem)
Q_DECLARE_METATYPE(QList<TrackListModel::TrackItem>)
