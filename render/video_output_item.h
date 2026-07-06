#ifndef VIDEO_OUTPUT_ITEM_H
#define VIDEO_OUTPUT_ITEM_H

#include <QImage>
#include <QPointer>
#include <QQuickFramebufferObject>

class VideoFrameSource;

class VideoOutputItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(QObject *frameSource READ frameSource WRITE setFrameSource NOTIFY frameSourceChanged)

public:
    explicit VideoOutputItem(QQuickItem *parent = nullptr);

    Renderer *createRenderer() const override;

    QObject *frameSource() const;
    void setFrameSource(QObject *source);

    QImage currentFrame() const;

signals:
    void frameSourceChanged();

private:
    QPointer<VideoFrameSource> m_frameSource;
};

#endif // VIDEO_OUTPUT_ITEM_H
