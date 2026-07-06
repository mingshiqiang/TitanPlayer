#include "video_output_item.h"

#include "video_frame_source.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QQuickWindow>

namespace {
const char *kVertexShader = R"(
attribute highp vec2 position;
attribute highp vec2 texCoord;
varying highp vec2 vTexCoord;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    vTexCoord = texCoord;
}
)";

const char *kFragmentShader = R"(
uniform sampler2D tex;
varying highp vec2 vTexCoord;
void main() {
    gl_FragColor = texture2D(tex, vTexCoord);
}
)";

class VideoRenderer final : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    VideoRenderer()
    {
        initializeOpenGLFunctions();
    }

    ~VideoRenderer() override
    {
        delete m_texture;
        delete m_program;
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        auto *videoItem = static_cast<VideoOutputItem *>(item);
        m_itemSize = videoItem->size().toSize();
        m_pendingFrame = videoItem->currentFrame();
    }

    void render() override
    {
        glClearColor(0.04f, 0.04f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!m_pendingFrame.isNull()) {
            uploadFrame(m_pendingFrame);
        }

        if (!m_texture || !m_program || m_itemSize.isEmpty()) {
            update();
            return;
        }

        QRect viewport = letterboxViewport();
        glViewport(viewport.x(), viewport.y(), viewport.width(), viewport.height());

        static const GLfloat vertices[] = {
            -1.0f, -1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 0.0f,
        };

        m_program->bind();
        m_texture->bind(0);
        m_program->setUniformValue("tex", 0);

        const int pos = m_program->attributeLocation("position");
        const int tex = m_program->attributeLocation("texCoord");
        m_program->enableAttributeArray(pos);
        m_program->enableAttributeArray(tex);
        m_program->setAttributeArray(pos, GL_FLOAT, vertices, 2, 4 * sizeof(GLfloat));
        m_program->setAttributeArray(tex, GL_FLOAT, vertices + 2, 2, 4 * sizeof(GLfloat));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        m_program->disableAttributeArray(pos);
        m_program->disableAttributeArray(tex);
        m_texture->release();
        m_program->release();

        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, format);
    }

private:
    void ensureProgram()
    {
        if (m_program) {
            return;
        }
        m_program = new QOpenGLShaderProgram();
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader);
        m_program->link();
    }

    void uploadFrame(const QImage &frame)
    {
        ensureProgram();

        QImage rgba = frame.convertToFormat(QImage::Format_RGBA8888);
        if (!m_texture || m_texture->width() != rgba.width() || m_texture->height() != rgba.height()) {
            delete m_texture;
            m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
            m_texture->setSize(rgba.width(), rgba.height());
            m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
            m_texture->setMinificationFilter(QOpenGLTexture::Linear);
            m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
            m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
            m_texture->allocateStorage();
        }
        m_texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, rgba.constBits());
        m_videoSize = rgba.size();
    }

    QRect letterboxViewport() const
    {
        if (m_videoSize.isEmpty()) {
            return QRect(QPoint(0, 0), m_itemSize);
        }

        const QSize scaled = m_videoSize.scaled(m_itemSize, Qt::KeepAspectRatio);
        const int x = (m_itemSize.width() - scaled.width()) / 2;
        const int y = (m_itemSize.height() - scaled.height()) / 2;
        return QRect(x, y, scaled.width(), scaled.height());
    }

    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLTexture *m_texture = nullptr;
    QImage m_pendingFrame;
    QSize m_itemSize;
    QSize m_videoSize;
};
}

VideoOutputItem::VideoOutputItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer *VideoOutputItem::createRenderer() const
{
    return new VideoRenderer();
}

QObject *VideoOutputItem::frameSource() const
{
    return m_frameSource;
}

void VideoOutputItem::setFrameSource(QObject *source)
{
    auto *frameSource = qobject_cast<VideoFrameSource *>(source);
    if (m_frameSource == frameSource) {
        return;
    }

    if (m_frameSource) {
        disconnect(m_frameSource, nullptr, this, nullptr);
    }

    m_frameSource = frameSource;
    if (m_frameSource) {
        connect(m_frameSource, &VideoFrameSource::frameReady, this, &VideoOutputItem::update);
    }

    emit frameSourceChanged();
    update();
}

QImage VideoOutputItem::currentFrame() const
{
    return m_frameSource ? m_frameSource->frame() : QImage();
}
