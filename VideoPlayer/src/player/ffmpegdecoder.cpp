/**
 * @brief FFmpeg decoder
 * @anchor Ho 229
 * @date 2021/4/13
 */

#include "config.h"
#include "ffmpegdecoder.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QScopeGuard>
#include <QMutexLocker>

#define FUNC_ERROR qCritical() << __FUNCTION__
#define FFMPEG_ERROR(x) FUNC_ERROR << ":" << __LINE__ \
                        << ":" << av_make_error_string(m_errorBuf, sizeof (m_errorBuf), x)
#define SET_AVTIME(x) m_videoTime = x; \
                      m_audioTime = x;

/**
 * @ref ffmpeg.c line:181 : static void sub2video_copy_rect()
 */
static void mergeSubtitle(uint8_t *dst, int dst_linesize, int w, int h,
                          AVSubtitleRect *r);
template <typename T>
static void findStreams(const AVFormatContext *format, AVMediaType type, QList<T> &list);
inline static bool shouldDecode(const QContiguousCache<AVFrame *> &cache,
                                AVRational timebase, qreal current);

FFmpegDecoder::FFmpegDecoder(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<State>();

    av_log_set_level(AV_LOG_INFO);

    m_videoCache.setCapacity(VIDEO_CACHE_SIZE);
    m_audioCache.setCapacity(AUDIO_CACHE_SIZE);
    m_subtitleCache.setCapacity(SUBTITLE_CACHE_SIZE);
}

FFmpegDecoder::~FFmpegDecoder()
{
    this->release();
}

int FFmpegDecoder::activeVideoTrack() const
{
    if(!m_videoStream)
        return -1;

    return m_videoIndexes.indexOf(m_videoStream->index);
}

int FFmpegDecoder::activeAudioTrack() const
{
    if(!m_audioStream)
        return -1;

    return m_audioIndexes.indexOf(m_audioStream->index);
}

int FFmpegDecoder::activeSubtitleTrack() const
{
    return m_subtitleIndex;
}

void FFmpegDecoder::setActiveVideoTrack(int index)
{
    m_runnable = true;

    if(m_state == Closed || index >= m_videoIndexes.size())
        return;

    if(m_videoCodecContext && m_videoStream)
    {
        if(index >= 0 && m_videoStream->index == m_videoIndexes[index])
            return;

        this->closeCodecContext(m_videoStream, m_videoCodecContext);
        if(m_swsContext)
        {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }
    }

    this->clearCache();
    SET_AVTIME(-1);

    // Initialize video codec context
    if(index < 0 || !this->openCodecContext(m_videoStream, m_videoCodecContext,
                                            AVMEDIA_TYPE_VIDEO, m_videoIndexes[index]))
        return;

    if(m_videoCodecContext->pix_fmt != AV_PIX_FMT_YUV420P &&
        m_videoCodecContext->pix_fmt != AV_PIX_FMT_YUV444P)
    {
        m_swsContext = sws_getContext(m_videoCodecContext->width,
                                      m_videoCodecContext->height,
                                      m_videoCodecContext->pix_fmt,
                                      m_videoCodecContext->width,
                                      m_videoCodecContext->height,
                                      AV_PIX_FMT_YUV420P,
                                      SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    emit activeVideoTrackChanged(index);
}

void FFmpegDecoder::setActiveAudioTrack(int index)
{
    m_runnable = true;

    if(m_state == Closed || index >= m_audioIndexes.size())
        return;

    if(m_audioCodecContext && m_audioStream)
    {
        if(index >= 0 && m_audioStream->index == m_audioIndexes[index])
            return;

        this->closeCodecContext(m_audioStream, m_audioCodecContext);
        if(m_swrContext)
            swr_free(&m_swrContext);
    }

    this->clearCache();
    SET_AVTIME(-1);

    // Initialize audio codec context
    if(index < 0 || !this->openCodecContext(m_audioStream, m_audioCodecContext,
                                             AVMEDIA_TYPE_AUDIO, m_audioIndexes[index]))
        return;

    if(m_audioCodecContext->sample_fmt != AV_SAMPLE_FMT_S16 ||
        m_audioCodecContext->ch_layout.nb_channels != 2)
    {
        AVChannelLayout dest;
        av_channel_layout_default(&dest, 2);
        swr_alloc_set_opts2(&m_swrContext,
                            &dest,
                            AV_SAMPLE_FMT_S16,
                            m_audioCodecContext->sample_rate,
                            &m_audioCodecContext->ch_layout,
                            m_audioCodecContext->sample_fmt,
                            m_audioCodecContext->sample_rate,
                            0, nullptr);
        swr_init(m_swrContext);
    }

    emit activeAudioTrackChanged(index);
}

void FFmpegDecoder::setActiveSubtitleTrack(int index)
{
    m_runnable = true;

    if(m_state == Closed || index >= m_subtitleIndexes.size() || m_subtitleIndex == index)
        return;

    if(m_subtitleCodecContext && m_subtitleStream)
        this->closeCodecContext(m_subtitleStream, m_subtitleCodecContext);
    else if(m_buffersrcContext && m_buffersinkContext)
        this->closeSubtitleFilter();

    m_subtitleIndex = -1;

    this->clearCache();
    SET_AVTIME(-1);

    if(index < 0 || !m_url.isLocalFile() || !m_videoStream)
        return;

    const AVRational timeBase = m_videoStream->time_base;
    const AVRational pixelAspect = m_videoCodecContext->sample_aspect_ratio;
    const QString args = QString::asprintf(
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        m_videoCodecContext->width, m_videoCodecContext->height,
        m_videoCodecContext->pix_fmt, timeBase.num, timeBase.den,
        pixelAspect.num, pixelAspect.den);

    auto makeFilterDesc = [this](const QString fileName, int index) -> QString {
        return QString("subtitles=filename='%1':original_size=%2x%3:si=%4")
            .arg(fileName)
            .arg(m_videoCodecContext->width)
            .arg(m_videoCodecContext->height)
            .arg(index);
    };
    auto convertPath = [](QString fileName) -> QString {
#ifdef Q_OS_WIN
        fileName.replace('/', "\\\\");
        return fileName.insert(fileName.indexOf(":\\"), char('\\'));
#else
        return fileName;
#endif
    };

    m_subtitleIndex = index;
    if(m_subtitleIndexes[index].type() == QVariant::Int)
    {
        const int absoluteIndex = m_subtitleIndexes[index].toInt();
        if(av_find_best_stream(m_formatContext, AVMEDIA_TYPE_SUBTITLE,
                               absoluteIndex, -1, nullptr, 0) > 0)
        {
            QString subtitleFileName = m_url.toLocalFile();

            if(!this->openSubtitleFilter(args, makeFilterDesc(convertPath(subtitleFileName), absoluteIndex)))
                this->openCodecContext(m_subtitleStream, m_subtitleCodecContext,
                                       AVMEDIA_TYPE_SUBTITLE, absoluteIndex);
        }
    }
    else if(m_subtitleIndexes[index].type() == QVariant::String)
        this->openSubtitleFilter(args, makeFilterDesc(convertPath(m_subtitleIndexes[index].toString()), 0));

    emit activeSubtitleTrackChanged(index);
}

bool FFmpegDecoder::seekable() const
{
    // return m_formatContext ? m_formatContext->pb->seekable : false;
    return m_url.isLocalFile();
}

int FFmpegDecoder::duration() const
{
    if(!m_formatContext || !this->seekable())
        return 0;

    return static_cast<int>(m_formatContext->duration / AV_TIME_BASE);
}

int FFmpegDecoder::position() const
{
    if(m_videoTime >= 0)
        m_position = static_cast<int>(m_videoTime);
    else if(m_audioTime >= 0)
        m_position = static_cast<int>(m_audioTime);

    return m_state == Closed ? 0 : m_position;
}

QSize FFmpegDecoder::videoSize() const
{
    return m_videoCodecContext ? QSize(m_videoCodecContext->width, m_videoCodecContext->height) : QSize();
}

AVPixelFormat FFmpegDecoder::videoPixelFormat() const
{
    const auto originalFormat = m_videoCodecContext ? m_videoCodecContext->pix_fmt : AV_PIX_FMT_NONE;
    return m_swsContext ? AV_PIX_FMT_YUV420P : originalFormat;
}

int FFmpegDecoder::videoTrackCount() const
{
    return m_videoIndexes.size();
}

int FFmpegDecoder::audioTrackCount() const
{
    return m_audioIndexes.size();
}

int FFmpegDecoder::subtitleTrackCount() const
{
    return m_subtitleIndexes.size();
}

bool FFmpegDecoder::isBitmapSubtitleActived() const
{
    return m_subtitleCodecContext && m_subtitleCodecContext->pix_fmt == AV_PIX_FMT_PAL8;
}

void FFmpegDecoder::load()
{
    this->release();        // Reset

    int ret = 0;

    const QString url = m_url.isLocalFile() ? m_url.toLocalFile() : m_url.toString();

    // Open file
    // Note that FFmpeg accepts filename encoded in UTF-8
    if((ret = avformat_open_input(&m_formatContext, url.toUtf8().data(),
             nullptr, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        emit stateChanged(Closed);
        return;
    }

    // Find stream info
    if((ret = avformat_find_stream_info(m_formatContext, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        this->release();
        return;
    }

    // Print file infomation
    av_dump_format(m_formatContext, 0, m_formatContext->url, 0);

    findStreams(m_formatContext, AVMEDIA_TYPE_VIDEO, m_videoIndexes);

    findStreams(m_formatContext, AVMEDIA_TYPE_AUDIO, m_audioIndexes);

    findStreams(m_formatContext, AVMEDIA_TYPE_SUBTITLE, m_subtitleIndexes);
    if(m_url.isLocalFile())
    {
        const QFileInfo fileInfo(url);
        const QDir subtitleDir(fileInfo.absoluteDir());
        const QStringList subtitleFiles =
            subtitleDir.entryList({{"*.ass"}, {"*.srt"}, {"*.lrc"}}, QDir::Files)
                .filter(fileInfo.baseName());

        for(const auto &file : subtitleFiles)
            m_subtitleIndexes.append(subtitleDir.filePath(file));
    }

    m_state = Opened;

    this->setActiveVideoTrack(0);
    this->setActiveAudioTrack(0);
    this->setActiveSubtitleTrack(0);

    // If there is no video and audio or fps is invalid
    if(!(m_videoStream || m_audioStream) || qFuzzyIsNull(this->fps()))
    {
        qstrcpy(m_errorBuf, "There is no video and audio or fps is invalid");
        this->release();
        return;
    }

    m_isEnd = false;
    m_runnable = true;

    emit stateChanged(m_state);

    this->decode();
}

void FFmpegDecoder::release()
{
    if(m_state == Closed)
        return;

    this->setActiveVideoTrack(-1);
    this->setActiveAudioTrack(-1);
    this->setActiveSubtitleTrack(-1);

    avformat_close_input(&m_formatContext);

    m_position = 0;
    SET_AVTIME(-1);

    m_videoIndexes.clear();
    m_audioIndexes.clear();
    m_subtitleIndexes.clear();

    m_state = Closed;
    emit stateChanged(m_state);
}

void FFmpegDecoder::seek(int position)
{
    m_runnable = true;

    if(m_state == Closed || (!m_videoStream && !m_audioStream))
        return;

    // Clear frame cache
    this->clearCache();
    SET_AVTIME(-1);

    const AVStream *seekStream = m_videoStream ? m_videoStream : m_audioStream;
    const int flags = (position < m_position ? AVSEEK_FLAG_BACKWARD : 0) | AVSEEK_FLAG_FRAME;
    av_seek_frame(m_formatContext, seekStream->index, static_cast<qint64>
                  (position / av_q2d(seekStream->time_base)), flags);

    m_position = position;

    // Flush the codec buffers
    if(m_videoCodecContext)
        avcodec_flush_buffers(m_videoCodecContext);
    if(m_audioCodecContext)
        avcodec_flush_buffers(m_audioCodecContext);

    // runs on the same thread so doesn't need to be called by signal
    this->decode();
}

AVFrame *FFmpegDecoder::takeVideoFrame()
{
    if(m_state == Closed)
        return nullptr;

    AVFrame *frame = nullptr;
    m_mutex.lock();

    if(!m_videoCache.isEmpty())
    {
        frame = m_videoCache.takeFirst();
        m_videoTime = second(frame->pts, m_videoStream->time_base);
    }

    if(shouldDecode(m_videoCache, m_videoStream->time_base, m_videoTime) &&
        !m_isEnd && !m_isDecoding)
    {
        m_isDecoding = true;
        // Asynchronous call FFmpegDecoder::decode()
        QMetaObject::invokeMethod(this, &FFmpegDecoder::decode);
    }

    m_mutex.unlock();

    return frame;
}

qint64 FFmpegDecoder::takeAudioData(char *data, qint64 len)
{
    if(m_state == Closed || !len)
        return {};

    qint64 free = len;
    char *dest = data;

    m_mutex.lock();
    while(!m_audioCache.isEmpty() && m_audioCache.first()->linesize[0] <= free)
    {
        AVFrame *frame = m_audioCache.takeFirst();
        const int size = frame->linesize[0];

        m_audioTime = second(frame->pts, m_audioStream->time_base);

        memcpy(dest, frame->data[0], size);
        av_frame_free(&frame);

        free -= size;
        dest += size;
    }

    if(shouldDecode(m_audioCache, m_audioStream->time_base, m_videoTime) &&
        !m_isEnd && !m_isDecoding)
    {
        m_isDecoding = true;
        // Asynchronous call FFmpegDecoder::decode()
        QMetaObject::invokeMethod(this, &FFmpegDecoder::decode);
    }

    m_mutex.unlock();

    return len - free;
}

QSharedPointer<SubtitleFrame> FFmpegDecoder::takeSubtitleFrame()
{
    QMutexLocker locker(&m_mutex);

    while(!m_subtitleCache.isEmpty() && m_subtitleCache.first()->end < m_videoTime)
        m_subtitleCache.removeFirst();

    if(m_subtitleCache.isEmpty() || m_subtitleCache.first()->start > m_videoTime)
        return {};

    return m_subtitleCache.first();
}

const QAudioFormat FFmpegDecoder::audioFormat() const
{
    QAudioFormat format;

    if(m_audioCodecContext)
    {
        format.setCodec("audio/pcm");
        format.setChannelCount(2);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setSampleRate(m_audioCodecContext->sample_rate);
        format.setSampleSize(16);
    }

    return format;
}

qreal FFmpegDecoder::fps() const
{
    return m_videoStream ? av_q2d(m_videoStream->avg_frame_rate) : 0;
}

qreal FFmpegDecoder::diff() const
{
    return m_videoTime >= 0 && m_audioTime >= 0 ? m_audioTime - m_videoTime - AUDIO_DELAY : 0;
}

void FFmpegDecoder::decode()
{
    AVPacket *packet = av_packet_alloc();

    m_isDecoding = true;
    while(m_state == Opened && !this->isCacheFull() && m_runnable)
    {
        m_isEnd = av_read_frame(m_formatContext, packet);
        if(m_isEnd)
            break;

        // Video frame decode
        if(m_videoStream && packet->stream_index == m_videoStream->index &&
            !avcodec_send_packet(m_videoCodecContext, packet))
            this->decodeVideo();

        // Audio frame decode
        else if(m_audioStream && packet->stream_index == m_audioStream->index &&
                 !avcodec_send_packet(m_audioCodecContext, packet))
            this->decodeAudio();

        // Subtitle frame decode
        else if(m_subtitleStream && packet->stream_index == m_subtitleStream->index)
            this->decodeSubtitle(packet);

        av_packet_unref(packet);
    }

    m_isDecoding = false;
    av_packet_free(&packet);
}

void FFmpegDecoder::decodeVideo()
{
    AVFrame *frame = av_frame_alloc();
    if(avcodec_receive_frame(m_videoCodecContext, frame))
    {
        av_frame_free(&frame);
        return;
    }

    if(m_buffersrcContext && m_buffersinkContext)
    {
        AVFrame *filterFrame = av_frame_alloc();
        // FIXME: memory leak here
        if(av_buffersrc_add_frame_flags(
                m_buffersrcContext, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0
            && av_buffersink_get_frame(m_buffersinkContext, filterFrame) >= 0)
        {
            av_frame_free(&frame);
            frame = filterFrame;
        }
        else
            av_frame_free(&filterFrame);
    }

    if(m_swsContext)
    {
        AVFrame *swsFrame = av_frame_alloc();
        swsFrame->pts = frame->pts;
        swsFrame->width = frame->width;
        swsFrame->height = frame->height;
        swsFrame->format = AV_PIX_FMT_YUV420P;

        sws_scale_frame(m_swsContext, swsFrame, frame);
        av_frame_free(&frame);

        frame = swsFrame;
    }

    QMutexLocker locker(&m_mutex);
    m_videoCache.append(frame);
}

void FFmpegDecoder::decodeAudio()
{
    AVFrame *frame = av_frame_alloc();
    if(avcodec_receive_frame(m_audioCodecContext, frame))
    {
        av_frame_free(&frame);
        return;
    }

    if(m_swrContext)
    {
        AVFrame *swrFrame = av_frame_alloc();
        swrFrame->pts = frame->pts;
        swrFrame->nb_samples = frame->nb_samples;
        swrFrame->format = AV_SAMPLE_FMT_S16;
        av_channel_layout_default(&swrFrame->ch_layout, 2);

        av_frame_get_buffer(swrFrame, 0);

        swr_convert(m_swrContext, swrFrame->data, frame->nb_samples,
                    const_cast<const uint8_t **>(frame->data),
                    frame->nb_samples);

        av_frame_free(&frame);
        frame = swrFrame;
    }

    QMutexLocker locker(&m_mutex);
    m_audioCache.append(frame);
}

void FFmpegDecoder::decodeSubtitle(AVPacket *packet)
{
    int isGot = 0;
    AVSubtitle subtitle;
    auto cleanup = qScopeGuard([&] { avsubtitle_free(&subtitle); });

    if(avcodec_decode_subtitle2(m_subtitleCodecContext,&subtitle, &isGot, packet) < 0 ||
        !isGot ||
        subtitle.format != 0)   // why the fucking ffmpeg doesn't do the job
        return;

    auto frame = QSharedPointer<SubtitleFrame>(
        new SubtitleFrame(m_subtitleCodecContext->width, m_subtitleCodecContext->height));

    for(uint i = 0; i < subtitle.num_rects; ++i)
        mergeSubtitle(frame->image.bits(), frame->image.bytesPerLine(),
                      frame->image.width(), frame->image.height(),
                      subtitle.rects[i]);

    const auto duration =
        packet->duration > 0 ? second(packet->duration, m_subtitleStream->time_base) :
            SUBTITLE_DEFAULT_DURATION;

    frame->start = second(packet->pts, m_subtitleStream->time_base);
    frame->end = frame->start + duration;

    QMutexLocker locker(&m_mutex);

    if(!m_subtitleCache.isEmpty() && m_subtitleCache.last()->end > frame->start)
        m_subtitleCache.last()->end = frame->start;

    m_subtitleCache.append(std::move(frame));
}

void FFmpegDecoder::clearCache()
{
    QMutexLocker locker(&m_mutex);

    // Clear video cache
    AVFrame *frame = nullptr;
    while(!m_videoCache.isEmpty())
    {
        frame = m_videoCache.takeFirst();
        av_frame_free(&frame);
    }

    // Clear audio and subtitle cache
    while(!m_audioCache.isEmpty())
    {
        frame = m_audioCache.takeFirst();
        av_frame_free(&frame);
    }

    m_subtitleCache.clear();
}

bool FFmpegDecoder::openCodecContext(AVStream *&stream, AVCodecContext *&codecContext,
                                     AVMediaType type, int index)
{
    // Find stream
    int ret = 0;
    if ((ret = av_find_best_stream(m_formatContext, type, index, -1, nullptr, 0)) < 0)
    {
        FUNC_ERROR << "Could not find stream" << av_get_media_type_string(type);
        FFMPEG_ERROR(ret);
        return false;
    }

    stream = m_formatContext->streams[ret];

    // Find codec
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        FUNC_ERROR << "Cound not find codec" << av_get_media_type_string(type);
        return false;
    }

    // Allocate codec context
    if(!(codecContext = avcodec_alloc_context3(codec)))
    {
        FUNC_ERROR << "Failed to allocate codec context";
        return false;
    }

    codecContext->thread_count = 1;

    AVDictionary *opts = nullptr;
    if ((ret = avcodec_parameters_to_context(codecContext, stream->codecpar)) < 0)
    {
        FUNC_ERROR << "Failed to copy codec parameters to decoder context"
                   << av_get_media_type_string(type);
        FFMPEG_ERROR(ret);
        return false;
    }
    av_dict_set(&opts, "refcounted_frames", "0", 0);

    // Open codec and get the context
    if ((ret = avcodec_open2(codecContext, codec, &opts)) < 0)
    {
        FUNC_ERROR << "Failed to open codec" << av_get_media_type_string(type);
        FFMPEG_ERROR(ret);
        return false;
    }

    return true;
}

void FFmpegDecoder::closeCodecContext(AVStream *&stream, AVCodecContext *&codecContext)
{
    avcodec_flush_buffers(codecContext);
    avcodec_free_context(&codecContext);

    stream = nullptr;
}

bool FFmpegDecoder::openSubtitleFilter(const QString& args, const QString& filterDesc)
{
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *input = avfilter_inout_alloc();
    AVFilterGraph *filterGraph = avfilter_graph_alloc();

    auto cleanup = qScopeGuard([&output, &input] {
        avfilter_inout_free(&output);
        avfilter_inout_free(&input);
    });

    auto resetContext = [this] {
        m_buffersrcContext = nullptr;
        m_buffersinkContext = nullptr;
    };

    if(!output || !input || !filterGraph)
        return false;

    int ret = 0;
    // Create in filter using "arg"
    if((ret = avfilter_graph_create_filter(&m_buffersrcContext, buffersrc, "in",
                                            args.toUtf8().data(), nullptr, filterGraph)) < 0)
    {
        FFMPEG_ERROR(ret);
        return false;
    }

    // Create out filter
    if((ret = avfilter_graph_create_filter(&m_buffersinkContext, buffersink, "out",
                                            nullptr, nullptr, filterGraph)) < 0)
    {
        FFMPEG_ERROR(ret);
        return false;
    }

    output->name = av_strdup("in");
    output->next = nullptr;
    output->pad_idx = 0;
    output->filter_ctx = m_buffersrcContext;

    input->name = av_strdup("out");
    input->next = nullptr;
    input->pad_idx = 0;
    input->filter_ctx = m_buffersinkContext;

    if((ret = avfilter_graph_parse_ptr(filterGraph, filterDesc.toUtf8().data(),
                                 &input, &output, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        resetContext();
        return false;
    }

    if((ret = avfilter_graph_config(filterGraph, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        resetContext();
        return false;
    }

    return true;
}

void FFmpegDecoder::closeSubtitleFilter()
{
    avfilter_free(m_buffersrcContext);
    avfilter_free(m_buffersinkContext);

    m_buffersrcContext = nullptr;
    m_buffersinkContext = nullptr;
}

static void mergeSubtitle(uint8_t *dst, int dst_linesize, int w, int h,
                                  AVSubtitleRect *r)
{
    uint32_t *pal, *dst2;
    uint8_t *src, *src2;
    int x, y;

    if (r->type != SUBTITLE_BITMAP)
    {
        FUNC_ERROR << ": non-bitmap subtitle\n";
        return;
    }

    if (r->x < 0 || r->x + r->w > w || r->y < 0 || r->y + r->h > h)
    {
        FUNC_ERROR << ": rectangle overflowing\n";
        return;
    }

    dst += r->y * dst_linesize + r->x * 4;
    src = r->data[0];
    pal = reinterpret_cast<uint32_t *>(r->data[1]);
    for (y = 0; y < r->h; y++)
    {
        dst2 = reinterpret_cast<uint32_t *>(dst);
        src2 = src;

        for (x = 0; x < r->w; x++)
            *(dst2++) = pal[*(src2++)];

        dst += dst_linesize;
        src += r->linesize[0];
    }
}

template <typename T>
static void findStreams(const AVFormatContext *format, AVMediaType type, QList<T> &list)
{
    for(int i = 0; i < int(format->nb_streams); ++i)
        if(format->streams[i]->codecpar->codec_type == type)
            list.append(i);
}

inline static bool shouldDecode(const QContiguousCache<AVFrame *> &cache,
                                AVRational timebase, qreal current)
{
    if(cache.isEmpty())
        return true;

    return FFmpegDecoder::second(cache.last()->pts, timebase) - current < MIN_DECODED_DURATION;
}
