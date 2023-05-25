/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include "ffmpegdecoder.h"

#include <QOpenGLFunctions_4_4_Core>
#include <QOpenGLVertexArrayObject>
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QScopedArrayPointer>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

struct AVFrame;

class QOpenGLTexture;
class VideoPlayerPrivate;

class VideoRenderer : public QQuickFramebufferObject::Renderer,
                      protected QOpenGLFunctions_4_4_Core
{
public:
    VideoRenderer();
    ~VideoRenderer() Q_DECL_OVERRIDE;

    void render() Q_DECL_OVERRIDE;

    QOpenGLFramebufferObject *createFramebufferObject(
        const QSize &size) Q_DECL_OVERRIDE;

    void synchronize(QQuickFramebufferObject *) Q_DECL_OVERRIDE;

    void updateVideoFrame(AVFrame *frame);
    void updateSubtitleFrame(SubtitleFrame *frame);

private:
    QOpenGLTexture *m_texture[4] = { nullptr };    // [0]: Y, [1]: U, [2]: V, [3]: Subtitle

    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    QOpenGLShaderProgram m_program;

    QSize m_size, m_videoSize;
    QRect m_viewRect;
    QOpenGLTexture::PixelFormat m_pixelFormat;

    quint8 m_flags;

    AVFrame *m_frame = nullptr;
    SubtitleFrame *m_subtitle = nullptr;
    QScopedArrayPointer<const GLubyte> m_dummySubtitle;

    bool m_textureAlloced = false;

    void updateVideoTextureData();
    void updateSubtitleTextureData();

    void resize();

    void initializeProgram();

    void setupTexture();
    void allocateTexture(const QSize sizes[3]);
    void updateSubtitleTexture(const QSize &size);

    void destoryTexture();
};

#endif // VIDEORENDERER_H
