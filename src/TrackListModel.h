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
        PathRole
    };
    Q_ENUM(TrackRoles)

    struct TrackItem {
        QString title;
        QString subtitle;
        QString path;
        QUrl source;
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

signals:
    void countChanged();

private:
    QList<TrackItem> m_tracks;
};
