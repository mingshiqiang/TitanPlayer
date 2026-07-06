#ifndef VIDEO_FRAME_SOURCE_H
#define VIDEO_FRAME_SOURCE_H

#include <QImage>
#include <QMutex>
#include <QObject>

class VideoFrameSource : public QObject {
    Q_OBJECT

public:
    explicit VideoFrameSource(QObject *parent = nullptr);

    void setFrame(const QImage &frame);
    void clear();
    QImage frame() const;

signals:
    void frameReady();
    void frameCleared();

private:
    mutable QMutex m_mutex;
    QImage m_frame;
};

#endif // VIDEO_FRAME_SOURCE_H
