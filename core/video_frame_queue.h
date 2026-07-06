#ifndef VIDEO_FRAME_QUEUE_H
#define VIDEO_FRAME_QUEUE_H

#include <QByteArray>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QWaitCondition>

struct VideoFrame {
    QImage image;
    double pts = 0.0;
};

struct AudioChunk {
    QByteArray data;
    double pts = 0.0;
};

class VideoFrameQueue {
public:
    explicit VideoFrameQueue(int maxSize = 3) : m_maxSize(maxSize) {}

    void push(const VideoFrame &frame)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_queue.dequeue();
        }
        m_queue.enqueue(frame);
        m_cond.wakeOne();
    }

    bool pop(VideoFrame &frame, unsigned long timeout = 50)
    {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty() && timeout > 0) {
            m_cond.wait(&m_mutex, timeout);
        }
        if (m_queue.isEmpty()) {
            return false;
        }
        frame = m_queue.dequeue();
        return true;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
    }

    int size()
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

private:
    QQueue<VideoFrame> m_queue;
    QMutex m_mutex;
    QWaitCondition m_cond;
    int m_maxSize = 3;
};

class AudioChunkQueue {
public:
    explicit AudioChunkQueue(int maxSize = 10) : m_maxSize(maxSize) {}

    void push(const AudioChunk &chunk)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_queue.dequeue();
        }
        m_queue.enqueue(chunk);
        m_cond.wakeOne();
    }

    bool pop(AudioChunk &chunk, unsigned long timeout = 50)
    {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty() && timeout > 0) {
            m_cond.wait(&m_mutex, timeout);
        }
        if (m_queue.isEmpty()) {
            return false;
        }
        chunk = m_queue.dequeue();
        return true;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
    }

    int size()
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

private:
    QQueue<AudioChunk> m_queue;
    QMutex m_mutex;
    QWaitCondition m_cond;
    int m_maxSize = 10;
};

#endif // VIDEO_FRAME_QUEUE_H
