/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#include "videorenderer.h"
#include "videoplayer_p.h"

#include <QQuickWindow>
#include <QOpenGLTexture>
#include <QGenericMatrix>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLFramebufferObjectFormat>

static const GLfloat vertices[] = {
    // vertex   texCoord
    1, 1,       1, 1,       // right top
    1, -1,      1, 0,       // right bottom
    -1, -1,     0, 0,       // left bottom
    -1, 1,      0, 1,       // left top
};

/**
 * @ref https://vocal.com/video/rgb-and-yuv-color-space-conversion/
 * @ref https://en.wikipedia.org/wiki/YCbC
 */
static const float colorinverseMatrices[][3 * 3] = {
    // BT.601
    {
        1, 0,        1.13983,
        1, -0.39465, -0.5806,
        1, 2.03211,  0
    },
    // BT.709
    {
        1, 0,       1.5748,
        1, -0.1873, -0.4681,
        1, 1.8556,  0
    },
    // BT.2020
    {
        1, 0,        1.4746,
        1, -0.16455, -0.5714,
        1, 1.8814,   0
    }
};

static QMatrix3x3 colorInverseMatrix(AVColorSpace space, AVColorRange range);
static void adjustColorRange(QMatrix3x3 &inverse, AVColorRange range);

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

    for(size_t i = 0; i < 4; ++i)
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
    const auto videoFormat = m_player_p->decoder->videoFormat();

    if(m_textureAlloced)
    {
        this->destoryTexture();
        m_textureAlloced = false;
    }

    auto updateColorMatrix = [&] {
        m_program.bind();
        // colorConversion
        m_program.setUniformValue(4, colorInverseMatrix(videoFormat.colorSpace, videoFormat.colorRange));
        m_program.release();
    };

    switch(videoFormat.pixelFormat)
    {
    case AV_PIX_FMT_YUV420P:            // YUV 420p 12bpp
        this->initializeTexture(videoFormat.pixelFormat, videoFormat.size);

        updateColorMatrix();
        m_textureAlloced = true;
        break;
    case AV_PIX_FMT_YUV444P:            // YUV 444P 24bpp
        this->initializeTexture(videoFormat.pixelFormat, videoFormat.size);

        updateColorMatrix();
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

    auto subtitle = m_player_p->decoder->takeSubtitleFrame();
    if(subtitle != m_subtitle)
    {
        m_subtitle = subtitle;
        if(subtitle)
        {
            if(m_texture[3]->width() != subtitle->image.width() || m_texture[3]->height() != subtitle->image.height())
                this->updateSubtitleTexture(subtitle->image.size());

            m_texture[3]->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, subtitle->image.constBits());
        }
        else        // Cleanup texture data
            m_texture[3]->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, m_dummySubtitle.data());
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
    for(int i = 0; i < 4; ++i)
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

void VideoRenderer::initializeTexture(AVPixelFormat format, const QSize &size)
{
    for(size_t i = 0; i < 3; ++i)
    {
        m_texture[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture[i]->setFormat(QOpenGLTexture::LuminanceFormat);
        m_texture[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_texture[i]->setWrapMode(QOpenGLTexture::ClampToEdge);

        if(format == AV_PIX_FMT_YUV420P && i > 0)
            m_texture[i]->setSize(size.width() / 2, size.height() / 2);
        else
            m_texture[i]->setSize(size.width(), size.height());

        m_texture[i]->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
    }

    m_texture[3] = new QOpenGLTexture(QOpenGLTexture::Target2D);

    // Temporary initialization, because the subtitle size is not known until the subtitle frame is decoded
    this->updateSubtitleTexture(size);
}

void VideoRenderer::updateSubtitleTexture(const QSize &size)
{
    if(m_texture[3]->isCreated())
        m_texture[3]->destroy();

    m_texture[3]->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_texture[3]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_texture[3]->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_texture[3]->setSize(size.width(), size.height());
    m_texture[3]->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

    m_dummySubtitle.reset(new GLubyte[size.width() * size.height() * 4]());
}

void VideoRenderer::resize()
{
    const QRect screenRect(QPoint(0, 0), m_size);
    QSize videoSize(m_player_p->decoder->videoFormat().size);

    if(!videoSize.isValid())
        return;

    m_viewRect.setSize(videoSize.scaled(m_size, Qt::KeepAspectRatio));
    m_viewRect.moveCenter(screenRect.center());
}

void VideoRenderer::destoryTexture()
{
    for(size_t i = 0; i < 4; ++i)
        delete m_texture[i];
}

static void adjustColorRange(QMatrix3x3 &inverse, AVColorRange range)
{
    auto mulPerLine = [&](const float vec[3]) {
        for(size_t i = 0; i < 3; ++i)
        {
            for(size_t j = 0; j < 3; ++j)
                inverse(i, j) *= vec[i];
        }
    };

    static const float jpeg[] = {255. / (255 - 0), 255. / (255 - 1), 255. / (255 - 1)};
    static const float mpeg[] = {255. / (235 - 16), 255. / (240 - 16), 255. / (240 - 16)};

    switch(range)
    {
    case AVCOL_RANGE_UNSPECIFIED:
    case AVCOL_RANGE_MPEG:
        mulPerLine(mpeg);
        break;
    case AVCOL_RANGE_JPEG:
        mulPerLine(jpeg);
        break;
    default:
        break;
    }
}

static QMatrix3x3 colorInverseMatrix(AVColorSpace space, AVColorRange range)
{
    QMatrix3x3 ret;

    switch(space)
    {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
        ret = QMatrix3x3(colorinverseMatrices[0]);
        break;
    case AVCOL_SPC_BT709:
        ret = QMatrix3x3(colorinverseMatrices[1]);
        break;
    case AVCOL_SPC_UNSPECIFIED:
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
        ret = QMatrix3x3(colorinverseMatrices[2]);
        break;
    default:
        FUNC_ERROR << "Unsupported color space: " << space << ", fallback to BT.2020";
        ret = QMatrix3x3(colorinverseMatrices[2]);
        break;
    }

    adjustColorRange(ret, range);
    return ret;
}
