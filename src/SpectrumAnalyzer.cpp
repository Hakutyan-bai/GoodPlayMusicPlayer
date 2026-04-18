#include "SpectrumAnalyzer.h"

#include <QAudioFormat>

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

template <typename SampleType>
QVector<qreal> extractSamplesTyped(const QAudioBuffer &buffer, qreal divisor, qreal offset = 0.0)
{
    const QAudioFormat format = buffer.format();
    const int channels = std::max(1, format.channelCount());
    const qsizetype frameCount = buffer.frameCount();
    if (frameCount <= 0) {
        return {};
    }

    const qsizetype targetFrames = std::min<qsizetype>(frameCount, 1024);
    const qsizetype firstFrame = frameCount - targetFrames;
    const SampleType *raw = buffer.constData<SampleType>();

    QVector<qreal> samples;
    samples.reserve(targetFrames);

    for (qsizetype frame = firstFrame; frame < frameCount; ++frame) {
        qreal mixed = 0.0;
        const qsizetype baseIndex = frame * channels;

        for (int channel = 0; channel < channels; ++channel) {
            mixed += (static_cast<qreal>(raw[baseIndex + channel]) - offset) / divisor;
        }

        samples.append(mixed / channels);
    }

    return samples;
}

int largestPowerOfTwo(int value)
{
    int power = 1;
    while ((power * 2) <= value) {
        power *= 2;
    }
    return power;
}

qreal clamp01(qreal value)
{
    return std::clamp(value, 0.0, 1.0);
}

} // namespace

QVector<qreal> SpectrumAnalyzer::analyze(const QAudioBuffer &buffer)
{
    QVector<qreal> monoSamples = extractMonoSamples(buffer);
    if (monoSamples.size() < 64) {
        return m_previous;
    }

    const QAudioFormat format = buffer.format();
    const int sampleRate = format.sampleRate();
    if (sampleRate <= 0) {
        return m_previous;
    }

    const int fftSize =
        largestPowerOfTwo(static_cast<int>(std::min<qsizetype>(monoSamples.size(), 512)));
    monoSamples = monoSamples.last(fftSize);

    QVector<qreal> windowed(fftSize, 0.0);
    for (int index = 0; index < fftSize; ++index) {
        const qreal multiplier = 0.5 * (1.0 - std::cos((2.0 * M_PI * index) / (fftSize - 1)));
        windowed[index] = monoSamples.at(index) * multiplier;
    }

    const int binCount = fftSize / 2;
    QVector<qreal> magnitudes(binCount, 0.0);

    for (int bin = 1; bin < binCount; ++bin) {
        qreal realPart = 0.0;
        qreal imaginaryPart = 0.0;

        for (int sampleIndex = 0; sampleIndex < fftSize; ++sampleIndex) {
            const qreal phase = (2.0 * M_PI * bin * sampleIndex) / fftSize;
            realPart += windowed.at(sampleIndex) * std::cos(phase);
            imaginaryPart -= windowed.at(sampleIndex) * std::sin(phase);
        }

        magnitudes[bin] = std::sqrt((realPart * realPart) + (imaginaryPart * imaginaryPart)) / fftSize;
    }

    const qreal minFrequency = 35.0;
    const qreal maxFrequency = std::min<qreal>(16000.0, sampleRate / 2.0);
    const qreal frequencyRatio = maxFrequency / minFrequency;

    QVector<qreal> result(BandCount, 0.0);

    for (int band = 0; band < BandCount; ++band) {
        const qreal startFrequency = minFrequency * std::pow(frequencyRatio, qreal(band) / BandCount);
        const qreal endFrequency = minFrequency * std::pow(frequencyRatio, qreal(band + 1) / BandCount);

        int startBin = std::max(1, static_cast<int>(std::floor((startFrequency * fftSize) / sampleRate)));
        int endBin = std::max(startBin + 1, static_cast<int>(std::ceil((endFrequency * fftSize) / sampleRate)));
        endBin = std::min(endBin, binCount);

        qreal energy = 0.0;
        int count = 0;
        for (int bin = startBin; bin < endBin; ++bin) {
            energy += magnitudes.at(bin);
            ++count;
        }

        if (count == 0) {
            result[band] = m_previous.at(band);
            continue;
        }

        energy /= count;

        const qreal bandFocus = 1.0 - std::abs((qreal(band) / (BandCount - 1)) - 0.55);
        const qreal weightedEnergy = energy * (160.0 + bandFocus * 140.0);
        const qreal normalized = std::log10(1.0 + weightedEnergy) / std::log10(1.0 + 220.0);
        const qreal boosted = std::pow(clamp01(normalized), 0.72);

        const qreal attack = (boosted > m_previous.at(band)) ? 0.45 : 0.16;
        const qreal smoothed = (m_previous.at(band) * (1.0 - attack)) + (boosted * attack);
        result[band] = clamp01(std::max(smoothed, m_previous.at(band) * 0.92));
    }

    m_previous = result;
    return result;
}

QVector<qreal> SpectrumAnalyzer::extractMonoSamples(const QAudioBuffer &buffer) const
{
    switch (buffer.format().sampleFormat()) {
    case QAudioFormat::UInt8:
        return extractSamplesTyped<quint8>(buffer, 128.0, 128.0);
    case QAudioFormat::Int16:
        return extractSamplesTyped<qint16>(buffer, 32768.0);
    case QAudioFormat::Int32:
        return extractSamplesTyped<qint32>(buffer, 2147483648.0);
    case QAudioFormat::Float:
        return extractSamplesTyped<float>(buffer, 1.0);
    case QAudioFormat::Unknown:
    default:
        return {};
    }
}
