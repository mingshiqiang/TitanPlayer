#include "audio_output.h"

#include <QAudioDevice>
#include <QMediaDevices>

AudioIODevice::AudioIODevice(AudioChunkQueue *queue, QObject *parent)
    : QIODevice(parent)
    , m_queue(queue)
{
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void AudioIODevice::start()
{
    m_buffer.clear();
}

void AudioIODevice::stop()
{
    m_buffer.clear();
}

qint64 AudioIODevice::bytesAvailable() const
{
    // QWindowsAudioSink only pulls when bytesAvailable() is positive.
    return m_buffer.size() + 4 * 1024 * 1024;
}

qint64 AudioIODevice::readData(char *data, qint64 maxlen)
{
    qint64 total = 0;

    while (total < maxlen && !m_buffer.isEmpty()) {
        const qint64 bytes = qMin<qint64>(m_buffer.size(), maxlen - total);
        memcpy(data + total, m_buffer.constData(), size_t(bytes));
        m_buffer.remove(0, int(bytes));
        total += bytes;
    }

    while (total < maxlen && m_queue) {
        AudioChunk chunk;
        if (!m_queue->pop(chunk, 0)) {
            break;
        }

        const qint64 bytes = qMin<qint64>(chunk.data.size(), maxlen - total);
        memcpy(data + total, chunk.data.constData(), size_t(bytes));
        total += bytes;

        if (bytes < chunk.data.size()) {
            m_buffer = chunk.data.mid(int(bytes));
        }
    }

    if (total < maxlen) {
        memset(data + total, 0, size_t(maxlen - total));
    }

    return maxlen;
}

qint64 AudioIODevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return 0;
}

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
{
}

AudioOutput::~AudioOutput()
{
    stop();
}

void AudioOutput::start(int sampleRate, int channels, AudioChunkQueue *queue)
{
    stop();

    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    QAudioFormat desired;
    desired.setSampleRate(sampleRate);
    desired.setChannelCount(channels);
    desired.setSampleFormat(QAudioFormat::Int16);

    if (device.isFormatSupported(desired)) {
        m_format = desired;
    } else {
        QAudioFormat preferred = device.preferredFormat();
        QAudioFormat int16Preferred = preferred;
        int16Preferred.setSampleFormat(QAudioFormat::Int16);
        m_format = device.isFormatSupported(int16Preferred) ? int16Preferred : preferred;
    }

    m_audioSink = new QAudioSink(device, m_format, this);
    m_audioSink->setBufferSize(8192 * 4);

    m_ioDevice = new AudioIODevice(queue, this);
    m_ioDevice->start();
    m_audioSink->start(m_ioDevice);
}

void AudioOutput::stop()
{
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    if (m_ioDevice) {
        m_ioDevice->stop();
        delete m_ioDevice;
        m_ioDevice = nullptr;
    }
}

void AudioOutput::suspend()
{
    if (m_audioSink) {
        m_audioSink->suspend();
    }
}

void AudioOutput::resume()
{
    if (m_audioSink) {
        m_audioSink->resume();
    }
}

void AudioOutput::setVolume(int volume)
{
    if (m_audioSink) {
        m_audioSink->setVolume(qBound(0.0, volume / 100.0, 1.0));
    }
}
