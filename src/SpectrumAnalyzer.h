#pragma once

#include <QAudioBuffer>
#include <QVector>

class SpectrumAnalyzer
{
public:
    static constexpr int BandCount = 48;

    QVector<qreal> analyze(const QAudioBuffer &buffer);

private:
    QVector<qreal> extractMonoSamples(const QAudioBuffer &buffer) const;

    QVector<qreal> m_previous { QVector<qreal>(BandCount, 0.0) };
};
