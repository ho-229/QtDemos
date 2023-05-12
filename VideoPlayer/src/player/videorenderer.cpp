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

static const GLfloat vertices[] = {
    // vertex   texCoord
    1, 1,       1, 1,       // right top
    1, -1,      1, 0,       // right bottom
    -1, -1,     0, 0,       // left bottom
    -1, 1,      0, 1,       // left top
};

VideoRenderer::VideoRenderer(VideoPlayerPrivate * const player_p)
    : m_player_p(player_p)
{
    if(!m_player_p)
        return;

    this->initializeOpenGLFunctions();

    this->initializeProgram();
}

VideoRenderer::~VideoRenderer()
{
    if(m_textureAlloced)
        this->destoryTexture();
}

void VideoRenderer::render()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(!m_textureAlloced)
        return;

    glViewport(m_viewRect.x(), m_viewRect.y(),
               m_viewRect.width(), m_viewRect.height());

    m_program.bind();
    m_vao.bind();

    for(size_t i = 0; i < 3; ++i)
        m_texture[i]->bind(i);

    glDrawArrays(GL_QUADS, 0, 4);

    m_vao.release();
    m_program.release();

    m_player_p->window()->resetOpenGLState();
}

QOpenGLFramebufferObject *VideoRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(8);
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

    if(m_textureAlloced)
    {
        this->destoryTexture();
        m_textureAlloced = false;
    }

    auto updatePixelFormat = [this](int isYuv420) {
        m_program.bind();
        m_program.setUniformValue(3, isYuv420);
        m_program.release();
    };

    switch(videoFormat)
    {
    case AV_PIX_FMT_YUV420P:            // YUV 420p 12bpp
        this->initializeTexture(true, videoSize);

        updatePixelFormat(true);
        m_textureAlloced = true;
        break;
    case AV_PIX_FMT_YUV444P:            // YUV 444P 24bpp
        this->initializeTexture(false, videoSize);

        updatePixelFormat(false);
        m_textureAlloced = true;
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

    for(size_t i = 0; i < 3; ++i)
    {
        options.setRowLength(m_frame->linesize[i]);
        m_texture[i]->setData(QOpenGLTexture::Luminance, QOpenGLTexture::UInt8,
                              reinterpret_cast<const void *>(m_frame->data[i]), &options);
    }
}

void VideoRenderer::initializeProgram()
{
    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex,":/vertex.vsh") ||
        !m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment.fsh"))
    {
        FUNC_ERROR << ": Add shader file failed.";
        return;
    }

    m_program.link();
    m_program.bind();

    // Set texture unit
    for(int i = 0; i < 3; ++i)
        m_program.setUniformValue(i, i);

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices));
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticRead);

    m_vao.create();
    m_vao.bind();

    // vertex
    m_program.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    m_program.enableAttributeArray(0);

    // texCoord
    m_program.setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    m_program.enableAttributeArray(1);

    m_vao.release();
    m_vbo.release();

    m_program.release();
}

void VideoRenderer::initializeTexture(bool isYuv420, const QSize &size)
{
    for(size_t i = 0; i < 3; ++i)
    {
        m_texture[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture[i]->setFormat(QOpenGLTexture::LuminanceFormat);
        m_texture[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_texture[i]->setWrapMode(QOpenGLTexture::ClampToEdge);

        if(isYuv420 && i > 0)
            m_texture[i]->setSize(size.width() / 2, size.height() / 2);
        else
            m_texture[i]->setSize(size.width(), size.height());

        m_texture[i]->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
    }
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

void VideoRenderer::destoryTexture()
{
    for(size_t i = 0; i < 3; ++i)
        delete m_texture[i];
}
