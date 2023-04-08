/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#include "videorenderer.h"
#include "videoplayer_p.h"
#include "subtitlerenderer.h"

#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLFramebufferObjectFormat>

VideoRenderer::VideoRenderer(VideoPlayerPrivate * const player_p)
    : m_player_p(player_p)
{
    if(!m_player_p)
        return;

    this->initializeOpenGLFunctions();

    m_decoder = m_player_p->decoder;

    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);

    this->initShader();
    this->initGeometry();
}

VideoRenderer::~VideoRenderer()
{
    destoryTexture(m_textureY);
    destoryTexture(m_textureU);
    destoryTexture(m_textureV);
}

void VideoRenderer::render()
{
    this->paint();
    m_player_p->window()->resetOpenGLState();
}

QOpenGLFramebufferObject *VideoRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(32);

    m_size = size;
    this->resize();

    return new QOpenGLFramebufferObject(size, format);
}

void VideoRenderer::synchronize(QQuickFramebufferObject *)
{
    // Update video info
    if(m_player_p->isVideoInfoChanged)
    {
        this->updateTextureInfo();
        this->resize();

        m_player_p->isVideoInfoChanged = false;
    }

    if(m_player_p->isUpdated)
    {
        this->updateTextureData(m_decoder->takeVideoFrame());
        m_player_p->isUpdated = false;
    }
}

void VideoRenderer::updateTextureInfo()
{
    const VideoInfo info(m_decoder->videoInfo());

    destoryTexture(m_textureY);
    destoryTexture(m_textureU);
    destoryTexture(m_textureV);
    m_textureAlloced = false;

    this->initTexture();

    switch(info.second)
    {
    case AV_PIX_FMT_YUV420P:        // YUV 420p 12bpp
        m_textureY->setSize(info.first.width(), info.first.height());
        m_textureY->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureU->setSize(info.first.width() / 2, info.first.height() / 2);
        m_textureU->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureV->setSize(info.first.width() / 2, info.first.height() / 2);
        m_textureV->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureAlloced = true;
        break;
    case AV_PIX_FMT_YUV444P:        // YUV 444P 24bpp
        m_textureY->setSize(info.first.width(), info.first.height());
        m_textureY->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureU->setSize(info.first.width(), info.first.height());
        m_textureU->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureV->setSize(info.first.width(), info.first.height());
        m_textureV->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureAlloced = true;
        break;
    default:
        FUNC_ERROR << ": Unknow pixel format" << info.second;
    }
}

void VideoRenderer::updateTextureData(AVFrame *frame)
{
    if(!frame)
        return;

    QOpenGLPixelTransferOptions options;
    options.setImageHeight(frame->height);

    options.setRowLength(frame->linesize[0]);
    m_textureY->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                   reinterpret_cast<const void *>(frame->data[0]), &options);
    options.setRowLength(frame->linesize[1]);
    m_textureU->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                   reinterpret_cast<const void *>(frame->data[1]), &options);
    options.setRowLength(frame->linesize[2]);
    m_textureV->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                   reinterpret_cast<const void *>(frame->data[2]), &options);

    av_frame_free(&frame);
}

void VideoRenderer::paint()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(m_player_p->state == VideoPlayer::Stopped || !m_textureAlloced)
        return;

    glViewport(m_viewRect.x(), m_viewRect.y(),
               m_viewRect.width(), m_viewRect.height());

    m_program.bind();

    m_modelMatHandle = m_program.uniformLocation("u_modelMatrix");
    m_viewMatHandle = m_program.uniformLocation("u_viewMatrix");
    m_projectMatHandle = m_program.uniformLocation("u_projectMatrix");
    m_verticesHandle = m_program.attributeLocation("qt_Vertex");
    m_texCoordHandle = m_program.attributeLocation("texCoord");

    // Vertex
    m_program.enableAttributeArray(m_verticesHandle);
    m_program.setAttributeArray(m_verticesHandle, m_vertices.constData());

    // fragment position
    m_program.enableAttributeArray(m_texCoordHandle);
    m_program.setAttributeArray(m_texCoordHandle, m_texcoords.constData());

    // MVP rectangle
    m_program.setUniformValue(m_modelMatHandle, m_modelMatrix);
    m_program.setUniformValue(m_viewMatHandle, m_viewMatrix);
    m_program.setUniformValue(m_projectMatHandle, m_projectionMatrix);

    // pixFmt
    m_program.setUniformValue("pixFmt", m_pixFmt);

    // Texture
    // Y
    glActiveTexture(GL_TEXTURE0);
    m_textureY->bind();

    // U
    glActiveTexture(GL_TEXTURE1);
    m_textureU->bind();

    // V
    glActiveTexture(GL_TEXTURE2);
    m_textureV->bind();

    m_program.setUniformValue("tex_y", 0);
    m_program.setUniformValue("tex_u", 1);
    m_program.setUniformValue("tex_v", 2);
    m_program.setUniformValue("tex_sub", 3);

    glDrawArrays(GL_TRIANGLE_FAN, 0, m_vertices.size());

    m_program.disableAttributeArray(m_verticesHandle);
    m_program.disableAttributeArray(m_texCoordHandle);
    m_program.release();
}

void VideoRenderer::initShader()
{
    if (!m_program.addShaderFromSourceFile(
            QOpenGLShader::Vertex,":/vertex.vsh") ||
        !m_program.addShaderFromSourceFile(
            QOpenGLShader::Fragment, ":/fragment.fsh"))
    {
        FUNC_ERROR << ": Add shader file failed.";
        return;
    }

    m_program.bindAttributeLocation("qt_Vertex", 0);
    m_program.bindAttributeLocation("texCoord", 1);

    m_program.link();
    m_program.bind();
}

void VideoRenderer::initTexture()
{
    // YUV 420p
    m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureY->setFormat(QOpenGLTexture::LuminanceFormat);
    //    mTexY->setFixedSamplePositions(false);
    m_textureY->setMinificationFilter(QOpenGLTexture::Nearest);
    m_textureY->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureY->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureU->setFormat(QOpenGLTexture::LuminanceFormat);
    //    mTexU->setFixedSamplePositions(false);
    m_textureU->setMinificationFilter(QOpenGLTexture::Nearest);
    m_textureU->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureU->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureV->setFormat(QOpenGLTexture::LuminanceFormat);
    //    mTexV->setFixedSamplePositions(false);
    m_textureV->setMinificationFilter(QOpenGLTexture::Nearest);
    m_textureV->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureV->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void VideoRenderer::initGeometry()
{
    m_vertices << QVector3D(-1, 1, 0.0f)
               << QVector3D(1, 1, 0.0f)
               << QVector3D(1, -1, 0.0f)
               << QVector3D(-1, -1, 0.0f);

    m_texcoords << QVector2D(0, 1)
                << QVector2D(1, 1)
                << QVector2D(1, 0)
                << QVector2D(0, 0);

    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(QVector3D(0.0f, 0.0f, 1.001f), QVector3D(0.0f, 0.0f, -5.0f),
                       QVector3D(0.0f, 1.0f, 0.0f));
    m_modelMatrix.setToIdentity();

    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.frustum(-1.0, 1.0, -1.0f, 1.0f, 1.0f, 100.0f);
}

void VideoRenderer::resize()
{
    const QRect screenRect(QPoint(0, 0), m_size);
    const QSize videoSize(m_decoder->videoInfo().first);

    if(!videoSize.isValid())
        return;

    m_viewRect.setSize(videoSize.scaled(m_size, Qt::KeepAspectRatio));
    m_viewRect.moveCenter(screenRect.center());

    m_player_p->subtitleRenderer->setX(m_viewRect.x());
    m_player_p->subtitleRenderer->setY(m_viewRect.y());
    m_player_p->subtitleRenderer->setSize(m_viewRect.size());
}

void VideoRenderer::destoryTexture(QOpenGLTexture *&texture)
{
    if (texture)
    {
        if (texture->isBound())
            texture->release();

        if (texture->isCreated())
            texture->destroy();

        delete texture;
        texture = nullptr;
    }
}
