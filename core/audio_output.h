#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <QAudioFormat>
#include <QAudioSink>
#include <QByteArray>
#include <QIODevice>
#include <QObject>

#include "video_frame_queue.h"

class AudioIODevice : public QIODevice {
    Q_OBJECT

public:
    explicit AudioIODevice(AudioChunkQueue *queue, QObject *parent = nullptr);

    void start();
    void stop();

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;

private:
    AudioChunkQueue *m_queue = nullptr;
    QByteArray m_buffer;
};

class AudioOutput : public QObject {
    Q_OBJECT

public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput() override;

    void start(int sampleRate, int channels, AudioChunkQueue *queue);
    void stop();
    void suspend();
    void resume();
    void setVolume(int volume);

    const QAudioFormat &format() const { return m_format; }

private:
    QAudioSink *m_audioSink = nullptr;
    AudioIODevice *m_ioDevice = nullptr;
    QAudioFormat m_format;
};

#endif // AUDIO_OUTPUT_H
