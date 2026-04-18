#pragma once

#include "SpectrumAnalyzer.h"
#include "TrackListModel.h"

#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include <QVariantList>

class MusicPlayerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TrackListModel *trackModel READ trackModel CONSTANT)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentSubtitle READ currentSubtitle NOTIFY currentTrackChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QUrl coverArtUrl READ coverArtUrl NOTIFY coverArtUrlChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QVariantList spectrumValues READ spectrumValues NOTIFY spectrumValuesChanged)

public:
    explicit MusicPlayerController(QObject *parent = nullptr);

    TrackListModel *trackModel();
    int currentIndex() const;
    QString currentTitle() const;
    QString currentSubtitle() const;
    qint64 position() const;
    qint64 duration() const;
    bool playing() const;
    qreal volume() const;
    QUrl coverArtUrl() const;
    QString errorMessage() const;
    QVariantList spectrumValues() const;

    Q_INVOKABLE void openFiles();
    Q_INVOKABLE void openFolder();
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void playAt(int index);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void setPosition(qint64 position);
    Q_INVOKABLE void setVolume(qreal volume);
    Q_INVOKABLE QString formatTime(qint64 milliseconds) const;

signals:
    void currentIndexChanged();
    void currentTrackChanged();
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void playingChanged();
    void volumeChanged();
    void coverArtUrlChanged();
    void errorMessageChanged();
    void spectrumValuesChanged();

private:
    void updateCoverArt();
    void setCoverArtUrl(const QUrl &url);
    QUrl createGeneratedCoverArt() const;
    QUrl saveCoverArtImage(const QImage &image, const QString &suffix) const;
    void appendSources(const QStringList &filePaths);
    QStringList audioNameFilters() const;
    QString audioDialogFilter() const;
    QVariantList toVariantList(const QVector<qreal> &values) const;
    void setErrorMessage(const QString &message);

    QMediaPlayer m_player;
    QAudioOutput m_audioOutput;
    QAudioBufferOutput m_audioBufferOutput;
    TrackListModel m_trackModel;
    SpectrumAnalyzer m_spectrumAnalyzer;
    QVariantList m_spectrumValues;
    int m_currentIndex = -1;
    QUrl m_coverArtUrl;
    QString m_errorMessage;
};
