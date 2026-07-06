#include "video_frame_source.h"

#include <QMutexLocker>

VideoFrameSource::VideoFrameSource(QObject *parent)
    : QObject(parent)
{
}

void VideoFrameSource::setFrame(const QImage &frame)
{
    {
        QMutexLocker locker(&m_mutex);
        m_frame = frame;
    }
    emit frameReady();
}

void VideoFrameSource::clear()
{
    {
        QMutexLocker locker(&m_mutex);
        m_frame = QImage();
    }
    emit frameCleared();
    emit frameReady();
}

QImage VideoFrameSource::frame() const
{
    QMutexLocker locker(&m_mutex);
    return m_frame;
}
