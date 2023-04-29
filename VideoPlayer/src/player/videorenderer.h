/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H
#include <QOpenGLFunctions_3_3_Core>
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>

struct AVFrame;

class QOpenGLTexture;
class VideoPlayerPrivate;

class VideoRenderer : public QQuickFramebufferObject::Renderer,
                      protected QOpenGLFunctions_3_3_Core
{
public:
    VideoRenderer(VideoPlayerPrivate * const player_p);
    ~VideoRenderer() Q_DECL_OVERRIDE;

    void render() Q_DECL_OVERRIDE;

    QOpenGLFramebufferObject *createFramebufferObject(
        const QSize &size) Q_DECL_OVERRIDE;

    void synchronize(QQuickFramebufferObject *) Q_DECL_OVERRIDE;

private:
    QOpenGLTexture *m_textureY = nullptr;
    QOpenGLTexture *m_textureU = nullptr;
    QOpenGLTexture *m_textureV = nullptr;

    VideoPlayerPrivate * const m_player_p = nullptr;

    QOpenGLShaderProgram m_program;

    QPair<int, QVector<QVector3D>> m_vertices;
    QPair<int, QVector<QVector2D>> m_texcoords;

    QPair<int, QMatrix4x4> m_modelMatrix;
    QPair<int, QMatrix4x4> m_viewMatrix;
    QPair<int, QMatrix4x4> m_projectionMatrix;
    QPair<int, GLint> m_pixelFormat;             // 0 is YUV420, 1 is YUV444
    int m_texY, m_texU, m_texV;

    QSize m_size;
    QRect m_viewRect;

    AVFrame *m_frame = nullptr;

    bool m_textureAlloced = false;

    void updateTexture();
    void updateTextureData();

    void initShader();
    void initTexture();
    void initGeometry();

    void paint();
    void resize();

    static void destoryTexture(QOpenGLTexture *&texture);
};

#endif // VIDEORENDERER_H
