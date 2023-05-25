/**
 * @brief Video Renderer
 * @anchor Ho 229
 * @date 2021/4/14
 */

#include "videorenderer.h"

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

enum Flag
{
    VideoFrameUpdate = 1,
    SubtitleFrameUpdate = 2,
};

static QMatrix3x3 colorInverseMatrix(AVColorSpace space, AVColorRange range);
static void adjustColorRange(QMatrix3x3 &inverse, AVColorRange range);

VideoRenderer::VideoRenderer()
{
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

    for(int i = 3; i > -1; --i)
        m_texture[i]->release(i);

    m_vao.release();
    m_program.release();
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
    if(m_flags & VideoFrameUpdate)
    {
        if(m_frame)
        {
            // Allocate texture when the first frame is encountered
            if(!m_textureAlloced)
            {
                m_videoSize = {m_frame->width, m_frame->height};
                this->setupTexture();
                this->resize();
            }

            this->updateVideoTextureData();
        }
        else if(m_textureAlloced)
            this->destoryTexture();
    }

    if(m_flags & SubtitleFrameUpdate && m_textureAlloced)
        this->updateSubtitleTextureData();

    m_flags = 0;
}

void VideoRenderer::updateVideoFrame(AVFrame *frame)
{
    m_frame = frame;
    m_flags |= VideoFrameUpdate;
}

void VideoRenderer::updateSubtitleFrame(SubtitleFrame *frame)
{
    if(frame == m_subtitle)
        return;

    m_subtitle = frame;
    m_flags |= SubtitleFrameUpdate;
}

void VideoRenderer::updateVideoTextureData()
{
    QOpenGLPixelTransferOptions options;
    options.setImageHeight(m_frame->height);

    for(size_t i = 0; i < 3; ++i)
    {
        options.setRowLength(m_pixelFormat == QOpenGLTexture::Luminance ?
                                 m_frame->linesize[i] : m_frame->linesize[i] / 2);
        m_texture[i]->setData(m_pixelFormat, QOpenGLTexture::UInt8,
                              reinterpret_cast<const void *>(m_frame->data[i]), &options);
    }

    av_frame_free(&m_frame);
}

void VideoRenderer::updateSubtitleTextureData()
{
    const void *data = m_dummySubtitle.data();
    if(m_subtitle)
    {
        if(m_texture[3]->width() != m_subtitle->image.width() || m_texture[3]->height() != m_subtitle->image.height())
            this->updateSubtitleTexture(m_subtitle->image.size());

        data = m_subtitle->image.constBits();
    }
    m_texture[3]->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, data);
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

void VideoRenderer::setupTexture()
{
    auto updateUniformValues = [&](int is10Bit) {
        m_program.bind();
        // colorConversion
        m_program.setUniformValue(4, colorInverseMatrix(m_frame->colorspace, m_frame->color_range));

        // is10Bit
        m_program.setUniformValue(5, is10Bit);
        m_program.release();
    };

    const QSize sizes420[3] = { m_videoSize, m_videoSize / 2,  m_videoSize / 2};
    const QSize sizes444[3] = { m_videoSize };

    switch(m_frame->format)
    {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUV420P10LE:
        m_pixelFormat = m_frame->format == AV_PIX_FMT_YUV420P10LE ?
                            QOpenGLTexture::LuminanceAlpha : QOpenGLTexture::Luminance;
        this->allocateTexture(sizes420);
        updateUniformValues(m_frame->format == AV_PIX_FMT_YUV420P10LE);
        break;
    case AV_PIX_FMT_YUV444P:
    case AV_PIX_FMT_YUV444P10LE:
        m_pixelFormat = m_frame->format == AV_PIX_FMT_YUV444P10LE ?
                            QOpenGLTexture::LuminanceAlpha : QOpenGLTexture::Luminance;
        this->allocateTexture(sizes444);
        updateUniformValues(m_frame->format == AV_PIX_FMT_YUV444P10LE);
        break;
    default:
        qCritical() << "Unsupport pixel format:" << m_frame->format;
    }
}

void VideoRenderer::allocateTexture(const QSize sizes[3])
{
    for(size_t i = 0; i < 3; ++i)
    {
        m_texture[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture[i]->setFormat(m_pixelFormat == QOpenGLTexture::Luminance ?
                                    QOpenGLTexture::LuminanceFormat : QOpenGLTexture::LuminanceAlphaFormat);
        m_texture[i]->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_texture[i]->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_texture[i]->setSize(sizes[i].width(), sizes[i].height());
        m_texture[i]->allocateStorage(m_pixelFormat, QOpenGLTexture::UInt8);
    }

    m_texture[3] = new QOpenGLTexture(QOpenGLTexture::Target2D);

    // Temporary initialization, because the subtitle size is not known until the subtitle frame is decoded
    this->updateSubtitleTexture(sizes[0]);      // sizes[0](Y channel) must be original size

    m_textureAlloced = true;
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
    if(!m_videoSize.isValid())
        return;

    const QRect screenRect(QPoint(0, 0), m_size);
    m_viewRect.setSize(m_videoSize.scaled(m_size, Qt::KeepAspectRatio));
    m_viewRect.moveCenter(screenRect.center());
}

void VideoRenderer::destoryTexture()
{
    for(size_t i = 0; i < 4; ++i)
        delete m_texture[i];

    m_textureAlloced = false;
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
