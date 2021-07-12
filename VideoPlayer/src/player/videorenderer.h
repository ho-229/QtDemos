/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEORENDERER_H
#define VIDEORENDERER_H

#include "ffmpegdecoder.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QQuickFramebufferObject>
#include <QOpenGLShaderProgram>

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
    FFmpegDecoder *m_decoder = nullptr;

    QOpenGLTexture *m_textureY = nullptr;
    QOpenGLTexture *m_textureU = nullptr;
    QOpenGLTexture *m_textureV = nullptr;

    VideoPlayerPrivate * const m_player_p = nullptr;

    QOpenGLShaderProgram m_program;

    QVector<QVector3D> m_vertices;
    QVector<QVector2D> m_texcoords;

    int m_modelMatHandle, m_viewMatHandle, m_projectMatHandle;
    int m_verticesHandle;
    int m_texCoordHandle;

    QMatrix4x4 m_modelMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;

    QSize m_size;
    QRect m_viewRect;

    GLint m_pixFmt = 0;
    bool m_textureAlloced = false;

    void updateTextureInfo();
    void updateTextureData(AVFrame *frame);

    inline void initShader();
    inline void initTexture();
    inline void initGeometry();

    inline void paint();
    inline void resize();

    static inline void destoryTexture(QOpenGLTexture *&texture);
};

#endif // VIDEORENDERER_H
