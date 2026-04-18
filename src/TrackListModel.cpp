#include "TrackListModel.h"

TrackListModel::TrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TrackListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_tracks.count();
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.count()) {
        return {};
    }

    const TrackItem &track = m_tracks.at(index.row());
    switch (role) {
    case TitleRole:
        return track.title;
    case SubtitleRole:
        return track.subtitle;
    case DurationRole:
        return track.duration;
    case SourceRole:
        return track.source;
    case PathRole:
        return track.path;
    default:
        return {};
    }
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    return {
        { TitleRole, "title" },
        { SubtitleRole, "subtitle" },
        { DurationRole, "duration" },
        { SourceRole, "source" },
        { PathRole, "path" }
    };
}

void TrackListModel::clear()
{
    if (m_tracks.isEmpty()) {
        return;
    }

    beginResetModel();
    m_tracks.clear();
    endResetModel();
    emit countChanged();
}

void TrackListModel::addTracks(const QList<TrackItem> &tracks)
{
    if (tracks.isEmpty()) {
        return;
    }

    const int beginRow = m_tracks.count();
    const int endRow = beginRow + tracks.count() - 1;

    beginInsertRows(QModelIndex(), beginRow, endRow);
    m_tracks.append(tracks);
    endInsertRows();
    emit countChanged();
}

TrackListModel::TrackItem TrackListModel::at(int index) const
{
    if (index < 0 || index >= m_tracks.count()) {
        return {};
    }

    return m_tracks.at(index);
}

bool TrackListModel::containsSource(const QUrl &source) const
{
    return std::any_of(m_tracks.cbegin(), m_tracks.cend(), [&source](const TrackItem &track) {
        return track.source == source;
    });
}

void TrackListModel::setTrackDuration(int index, qint64 duration)
{
    if (index < 0 || index >= m_tracks.count()) {
        return;
    }

    TrackItem &track = m_tracks[index];
    if (track.duration == duration) {
        return;
    }

    track.duration = duration;
    const QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, { DurationRole });
}
