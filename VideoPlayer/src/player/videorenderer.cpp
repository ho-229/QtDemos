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

    this->glDepthMask(GL_TRUE);
    this->glEnable(GL_TEXTURE_2D);

    this->initShader();
    this->initGeometry();
}

VideoRenderer::~VideoRenderer()
{
    this->destoryTexture(m_textureY);
    this->destoryTexture(m_textureU);
    this->destoryTexture(m_textureV);
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
    format.setMipmap(true);

    m_size = size;
    this->resize();

    return new QOpenGLFramebufferObject(size, format);
}

void VideoRenderer::synchronize(QQuickFramebufferObject *)
{
    // Destory texture
    if(m_player_p->isFormatUpdated && m_textureAlloced)
    {
        this->updateTexture();
        m_player_p->isFormatUpdated = false;
    }
    else if(m_player_p->isUpdated && (m_frame = m_player_p->decoder->takeVideoFrame()))
    {
        // Allocate texture when the first frame is encountered
        if(m_player_p->isFormatUpdated)
        {
            this->updateTexture();
            this->resize();

            m_player_p->isFormatUpdated = false;
        }

        this->updateTextureData();
        av_frame_free(&m_frame);

        m_player_p->isUpdated = false;
    }
}

void VideoRenderer::updateTexture()
{
    const auto videoFormat = m_player_p->decoder->videoPixelFormat();
    const auto videoSize = m_player_p->decoder->videoSize();

    destoryTexture(m_textureY);
    destoryTexture(m_textureU);
    destoryTexture(m_textureV);
    m_textureAlloced = false;

    switch(videoFormat)
    {
    case AV_PIX_FMT_YUV420P:        // YUV 420p 12bpp
        this->initTexture();

        m_textureY->setSize(videoSize.width(), videoSize.height());
        m_textureY->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureU->setSize(videoSize.width() / 2, videoSize.height() / 2);
        m_textureU->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureV->setSize(videoSize.width() / 2, videoSize.height() / 2);
        m_textureV->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_pixelFormat.second = 0;
        break;
    case AV_PIX_FMT_YUV444P:        // YUV 444P 24bpp
        this->initTexture();

        m_textureY->setSize(videoSize.width(), videoSize.height());
        m_textureY->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureU->setSize(videoSize.width(), videoSize.height());
        m_textureU->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_textureV->setSize(videoSize.width(), videoSize.height());
        m_textureV->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);

        m_pixelFormat.second = 1;
        break;
    default:
        break;
    }
}

void VideoRenderer::updateTextureData()
{
    if(!m_textureAlloced)
        return;

    QOpenGLPixelTransferOptions options;
    options.setImageHeight(m_frame->height);

    options.setRowLength(m_frame->linesize[0]);
    m_textureY->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                   reinterpret_cast<const void *>(m_frame->data[0]), &options);
    options.setRowLength(m_frame->linesize[1]);
    m_textureU->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                   reinterpret_cast<const void *>(m_frame->data[1]), &options);
    options.setRowLength(m_frame->linesize[2]);
    m_textureV->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                   reinterpret_cast<const void *>(m_frame->data[2]), &options);
}

void VideoRenderer::paint()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(!m_textureAlloced)
        return;

    glViewport(m_viewRect.x(), m_viewRect.y(),
               m_viewRect.width(), m_viewRect.height());

    m_program.bind();

    // Vertex
    m_program.enableAttributeArray(m_vertices.first);
    m_program.setAttributeArray(m_vertices.first, m_vertices.second.constData());

    // Fragment position
    m_program.enableAttributeArray(m_texcoords.first);
    m_program.setAttributeArray(m_texcoords.first, m_texcoords.second.constData());

    // MVP rectangle
    m_program.setUniformValue(m_modelMatrix.first, m_modelMatrix.second);
    m_program.setUniformValue(m_viewMatrix.first, m_viewMatrix.second);
    m_program.setUniformValue(m_projectionMatrix.first, m_projectionMatrix.second);

    // Pixel format
    m_program.setUniformValue(m_pixelFormat.first, m_pixelFormat.second);

    // Y
    m_textureY->bind(0);
    m_program.setUniformValue(m_texY, 0);

    // U
    m_textureU->bind(1);
    m_program.setUniformValue(m_texU, 1);

    // V
    m_textureV->bind(2);
    m_program.setUniformValue(m_texV, 2);

    glDrawArrays(GL_TRIANGLE_FAN, 0, m_vertices.second.size());

    m_program.disableAttributeArray(m_vertices.first);
    m_program.disableAttributeArray(m_texcoords.first);

    m_program.release();
}

void VideoRenderer::initShader()
{
    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex,":/vertex.vsh") ||
        !m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment.fsh"))
    {
        FUNC_ERROR << ": Add shader file failed.";
        return;
    }

    m_program.link();

    m_modelMatrix.first = m_program.uniformLocation("u_modelMatrix");
    m_viewMatrix.first = m_program.uniformLocation("u_viewMatrix");
    m_projectionMatrix.first = m_program.uniformLocation("u_projectMatrix");
    m_texY = m_program.uniformLocation("tex_y");
    m_texU = m_program.uniformLocation("tex_u");
    m_texV = m_program.uniformLocation("tex_v");

    m_vertices.first = m_program.attributeLocation("qt_Vertex");
    m_texcoords.first = m_program.attributeLocation("texCoord");
}

void VideoRenderer::initTexture()
{
    m_textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureY->setFormat(QOpenGLTexture::LuminanceFormat);
    m_textureY->setMinificationFilter(QOpenGLTexture::Nearest);
    m_textureY->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureY->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureU->setFormat(QOpenGLTexture::LuminanceFormat);
    m_textureU->setMinificationFilter(QOpenGLTexture::Nearest);
    m_textureU->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureU->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_textureV->setFormat(QOpenGLTexture::LuminanceFormat);
    m_textureV->setMinificationFilter(QOpenGLTexture::Nearest);
    m_textureV->setMagnificationFilter(QOpenGLTexture::Nearest);
    m_textureV->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_textureAlloced = true;
}

void VideoRenderer::initGeometry()
{
    m_vertices.second << QVector3D(-1, 1, 0.0f)
                      << QVector3D(1, 1, 0.0f)
                      << QVector3D(1, -1, 0.0f)
                      << QVector3D(-1, -1, 0.0f);

    m_texcoords.second << QVector2D(0, 1)
                       << QVector2D(1, 1)
                       << QVector2D(1, 0)
                       << QVector2D(0, 0);

    m_viewMatrix.second.setToIdentity();
    m_viewMatrix.second.lookAt(QVector3D(0.0f, 0.0f, 1.001f), QVector3D(0.0f, 0.0f, -5.0f),
                               QVector3D(0.0f, 1.0f, 0.0f));

    m_modelMatrix.second.setToIdentity();

    m_projectionMatrix.second.setToIdentity();
    m_projectionMatrix.second.frustum(-1.0, 1.0, -1.0f, 1.0f, 1.0f, 100.0f);
}

void VideoRenderer::resize()
{
    const QRect screenRect(QPoint(0, 0), m_size);
    const QSize videoSize(m_player_p->decoder->videoSize());

    if(!videoSize.isValid())
        return;

    m_viewRect.setSize(videoSize.scaled(m_size, Qt::KeepAspectRatio));
    m_viewRect.moveCenter(screenRect.center());

    m_player_p->subtitleRenderer->updateViewRect(m_viewRect);
}

void VideoRenderer::destoryTexture(QOpenGLTexture *&texture)
{
    if(!texture)
        return;

    if(texture->isBound())
        texture->release();

    if(texture->isCreated())
        texture->destroy();

    delete texture;
    texture = nullptr;
}
