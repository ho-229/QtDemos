/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include <QOpenGLFunctions_4_4_Core>
#include <QOpenGLVertexArrayObject>
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

struct AVFrame;

class QOpenGLTexture;
class VideoPlayerPrivate;

class VideoRenderer : public QQuickFramebufferObject::Renderer,
                      protected QOpenGLFunctions_4_4_Core
{
public:
    VideoRenderer(VideoPlayerPrivate *const player_p);
    ~VideoRenderer() Q_DECL_OVERRIDE;

    void render() Q_DECL_OVERRIDE;

    QOpenGLFramebufferObject *createFramebufferObject(
        const QSize &size) Q_DECL_OVERRIDE;

    void synchronize(QQuickFramebufferObject *) Q_DECL_OVERRIDE;

private:
    QOpenGLTexture *m_texture[3] = { nullptr };    // [0]: Y, [1]: U, [2]: V

    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    VideoPlayerPrivate *const m_player_p = nullptr;

    QOpenGLShaderProgram m_program;

    QSize m_size;
    QRect m_viewRect;

    AVFrame *m_frame = nullptr;

    bool m_textureAlloced = false;

    void updateTexture();
    void updateTextureData();

    void resize();

    void initializeProgram();

    void initializeTexture(bool isYuv420, const QSize &size);
    void destoryTexture();
};

#endif // VIDEORENDERER_H
