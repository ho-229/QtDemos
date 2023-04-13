/**
 * @brief FFmpeg decoder
 * @anchor Ho 229
 * @date 2021/4/13
 */

#include "ffmpegdecoder.h"

#include <QDir>
#include <QThread>
#include <QFileInfo>
#include <QMetaObject>
#include <QScopeGuard>
#include <QMutexLocker>

static void mergeSubtitle(uint8_t *dst, int dst_linesize, int w, int h,
                          AVSubtitleRect *r);
static int streamCount(const AVFormatContext *format, AVMediaType type);
static int findRelativeStream(const AVFormatContext *format,
                              int relativeIndex, AVMediaType type);

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

int FFmpegDecoder::audioTrackCount() const
{
    return m_formatContext ? streamCount(
               m_formatContext, AVMEDIA_TYPE_AUDIO) : -1;
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
        emit stateChanged(Error);
        return;
    }

    // Find stream info
    if((ret = avformat_find_stream_info(m_formatContext, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        emit stateChanged(Error);
        return;
    }

    // Print file infomation
    av_dump_format(m_formatContext, 0, m_formatContext->url, 0);

    // Initialize video codec context
    if((m_hasVideo = this->openCodecContext(m_videoStream, m_videoCodecContext,
                                             AVMEDIA_TYPE_VIDEO)))
    {
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
    }

    // Initialize audio codec context
    if((m_hasAudio = this->openCodecContext(m_audioStream, m_audioCodecContext,
                                             AVMEDIA_TYPE_AUDIO)))
    {
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
    }

    // If there is no video and audio
    if(!(m_hasVideo || m_hasAudio))
    {
        avformat_close_input(&m_formatContext);
        emit stateChanged(Error);
        return;
    }

    this->loadSubtitle();

    this->thread()->start();            // Start the decode thread

    m_isEnd = false;
    m_runnable = true;

    m_state = Opened;
    emit stateChanged(m_state);

    this->onDecode();
}

void FFmpegDecoder::release()
{
    if(m_state == Closed)
        return;

    m_runnable = false;

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    if(m_hasVideo)
        this->closeCodecContext(m_videoStream, m_videoCodecContext);

    if(m_hasAudio)
        this->closeCodecContext(m_audioStream, m_audioCodecContext);

    if(m_subtitleCodecContext)
        this->closeCodecContext(m_subtitleStream, m_subtitleCodecContext);

    if(m_buffersrcContext && m_buffersinkContext)
        this->closeSubtitleFilter();

    if(m_swrContext)
        swr_free(&m_swrContext);

    if(m_swsContext)
    {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    avformat_close_input(&m_formatContext);

    m_position = 0;

    m_isPtsUpdated = false;

    m_hasAudio = false;
    m_hasVideo = false;

    m_subtitleType = None;

    m_state = Closed;
    emit stateChanged(m_state);
}

void FFmpegDecoder::trackAudio(int index)
{
    m_runnable = true;

    if(index < 0 || index > this->audioTrackCount())
        return;

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    this->closeCodecContext(m_audioStream, m_audioCodecContext);
    this->openCodecContext(m_audioStream, m_audioCodecContext,
                           AVMEDIA_TYPE_AUDIO, index);

    this->onDecode();
}

void FFmpegDecoder::trackSubtitle(int index)
{
    m_runnable = true;

    if(index < 0 || index > this->subtitleTrackCount())
        return;

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    if(m_subtitleCodecContext)
    {
        this->closeCodecContext(m_subtitleStream, m_subtitleCodecContext);
        this->openCodecContext(m_subtitleStream, m_subtitleCodecContext,
                               AVMEDIA_TYPE_SUBTITLE, index);
    }
    else
    {
        if(m_buffersrcContext && m_buffersinkContext)
            this->closeSubtitleFilter();

        this->loadSubtitle(index);
    }

    this->onDecode();
}

int FFmpegDecoder::subtitleTrackCount() const
{
    if(!m_formatContext || m_subtitleType == None)
        return -1;

    int ret = 0;

    if((ret = streamCount(m_formatContext, AVMEDIA_TYPE_SUBTITLE)) > 0)
        return ret;

    const QFileInfo fileInfo(m_url.toLocalFile());

    const QDir subtitleDir(fileInfo.absoluteDir());

    const QStringList subtitleList =
        subtitleDir.entryList({{"*.ass"}, {"*.srt"}, {"*.lrc"}},
                              QDir::Files).filter(fileInfo.baseName());

    return subtitleList.isEmpty() ? -1 : subtitleList.size();
}

VideoInfo FFmpegDecoder::videoInfo() const
{
    if(!m_hasVideo && !m_videoCodecContext)
        return {{}, {AV_PIX_FMT_NONE}};

    AVPixelFormat format;
    if(m_videoCodecContext->pix_fmt != AV_PIX_FMT_YUV420P &&
        m_videoCodecContext->pix_fmt != AV_PIX_FMT_YUV444P)
        format = AV_PIX_FMT_YUV420P;
    else
        format = m_videoCodecContext->pix_fmt;

    return {{m_videoCodecContext->width, m_videoCodecContext->height}, format};
}

void FFmpegDecoder::seek(int position)
{
    if(m_state == Closed || m_position == position)
        return;

    // Clear frame cache
    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    const AVStream *seekStream = m_hasVideo ? m_videoStream : m_audioStream;
    av_seek_frame(m_formatContext, seekStream->index, static_cast<qint64>
                  (position / av_q2d(seekStream->time_base)),
                  AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    m_position = position;

    // Flush the codec buffers
    if(m_videoCodecContext)
        avcodec_flush_buffers(m_videoCodecContext);
    if(m_audioCodecContext)
        avcodec_flush_buffers(m_audioCodecContext);

    m_isPtsUpdated = false;
    m_runnable = true;

    emit seeked();

    // runs on the same thread so doesn't need to be called by signal
    this->onDecode();
}

void FFmpegDecoder::loadSubtitle(int index)
{
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

    auto convertPath = [](QString &fileName) -> QString& {
#ifdef Q_OS_WIN
        fileName.replace('/', "\\\\");
#endif
        return fileName.insert(fileName.indexOf(":\\"), char('\\'));
    };

    if(av_find_best_stream(m_formatContext, AVMEDIA_TYPE_SUBTITLE,
                            findRelativeStream(m_formatContext, index,
                                               AVMEDIA_TYPE_SUBTITLE), -1, nullptr, 0) > 0)
    {
        QString subtitleFileName = m_url.toLocalFile();

        if(this->openSubtitleFilter(args, makeFilterDesc(convertPath(subtitleFileName), index)))
            m_subtitleType = TextBased;
        // If is not text based subtitles
        else if(this->openCodecContext(m_subtitleStream, m_subtitleCodecContext,
                                        AVMEDIA_TYPE_SUBTITLE, index))
            m_subtitleType = Bitmap;
    }

    // If no found subtitle stream in video file
    else if(m_url.isLocalFile())
    {
        const QFileInfo fileInfo(m_url.toLocalFile());

        const QDir subtitleDir(fileInfo.absoluteDir());

        const QStringList subtitleList = subtitleDir
                .entryList({{"*.ass"}, {"*.srt"}, {"*.lrc"}}, QDir::Files)
                .filter(fileInfo.baseName());

        if(index < 0 || subtitleList.size() <= index)    // Out of range
            return;

        QString subtitleFileName = subtitleDir.absolutePath()
                                   + '/' + subtitleList.at(index);

        if(QFileInfo::exists(subtitleFileName))
        {
            if(this->openSubtitleFilter(args, makeFilterDesc(convertPath(subtitleFileName), 0)))
                m_subtitleType = TextBased;
        }
    }
}

AVFrame *FFmpegDecoder::takeVideoFrame()
{
    QMutexLocker locker(&m_mutex);

    if(m_state == Closed || m_videoCache.isEmpty())
        return nullptr;

    AVFrame *frame = m_videoCache.takeFirst();
    locker.unlock();

    m_videoTime = second(frame->pts, m_videoStream->time_base);

    if(m_isPtsUpdated)
        m_diff = second(m_audioPts, m_audioStream->time_base) - m_videoTime - AUDIO_DELAY;

    m_position = static_cast<int>(m_videoTime);

    if(m_videoCache.count() <= VIDEO_CACHE_SIZE / 2 && !m_isEnd)
        QMetaObject::invokeMethod(this, &FFmpegDecoder::onDecode);  // Asynchronous call FFmpegDecoder::decode()

    return frame;
}

qint64 FFmpegDecoder::takeAudioData(char *data, qint64 len)
{
    QMutexLocker locker(&m_mutex);
    if(m_state == Closed || m_audioCache.isEmpty() || !len)
        return {};

    qint64 free = len;
    char *dest = data;

    while(!m_audioCache.isEmpty() && m_audioCache.first()->linesize[0] <= free)
    {
        AVFrame *frame = m_audioCache.takeFirst();
        const int size = frame->linesize[0];

        m_audioPts = frame->pts;
        memcpy(dest, frame->data[0], size);
        av_frame_free(&frame);

        free -= size;
        dest += size;
    }

    locker.unlock();
    m_isPtsUpdated = true;

    if(m_audioCache.size() < AUDIO_CACHE_SIZE / 2 && !m_isEnd)
        QMetaObject::invokeMethod(this, &FFmpegDecoder::onDecode);  // Asynchronous call FFmpegDecoder::decode()

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

    format.setCodec("audio/pcm");
    format.setChannelCount(2);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(m_audioCodecContext->sample_rate);
    format.setSampleSize(16);

    return format;
}

qreal FFmpegDecoder::fps() const
{
    if(m_hasVideo)
        return av_q2d(m_videoStream->avg_frame_rate);

    return 0.0;
}

void FFmpegDecoder::onDecode()
{
    AVPacket *packet = av_packet_alloc();

    while(m_state == Opened && !this->isCacheFull() && m_runnable)
    {
        m_isEnd = av_read_frame(m_formatContext, packet);
        if(m_isEnd)
            break;

        // Video frame decode
        if(m_hasVideo && packet->stream_index == m_videoStream->index &&
            !avcodec_send_packet(m_videoCodecContext, packet))
        {
            AVFrame *frame = av_frame_alloc();
            if(frame && !avcodec_receive_frame(m_videoCodecContext, frame))
            {
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
            else
                av_frame_free(&frame);
        }

        // Audio frame decode
        else if(m_hasAudio && packet->stream_index == m_audioStream->index &&
                 !avcodec_send_packet(m_audioCodecContext, packet))
        {
            AVFrame *frame = av_frame_alloc();
            if(!avcodec_receive_frame(m_audioCodecContext, frame))
            {
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
            else
                av_frame_free(&frame);
        }

        // Subtitle frame decode
        else if(m_subtitleCodecContext && packet->stream_index == m_subtitleStream->index)
        {
            int isGet = 0;

            AVSubtitle subtitle;
            if(avcodec_decode_subtitle2(m_subtitleCodecContext,
                                         &subtitle, &isGet, packet) > 0 && isGet)
            {
                auto subtitleFrame = QSharedPointer<SubtitleFrame>(
                    new SubtitleFrame(m_subtitleCodecContext->width, m_subtitleCodecContext->height));

                for(uint i = 0; i < subtitle.num_rects; ++i)
                    mergeSubtitle(subtitleFrame->image.bits(), subtitleFrame->image.bytesPerLine(),
                                  subtitleFrame->image.width(), subtitleFrame->image.height(),
                                  subtitle.rects[i]);

                avsubtitle_free(&subtitle);

                const auto duration =
                    packet->duration > 0 ? second(packet->duration, m_subtitleStream->time_base) :
                        SUBTITLE_DEFAULT_DURATION;

                subtitleFrame->start = second(packet->pts, m_subtitleStream->time_base);
                subtitleFrame->end = subtitleFrame->start + duration;

                QMutexLocker locker(&m_mutex);

                if(!m_subtitleCache.isEmpty() && m_subtitleCache.last()->end > subtitleFrame->start)
                    m_subtitleCache.last()->end = subtitleFrame->start;

                m_subtitleCache.append(std::move(subtitleFrame));
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
}

void FFmpegDecoder::clearCache()
{
    // Clear video cache
    AVFrame *frame = nullptr;
    while(!m_videoCache.isEmpty())
    {
        frame = m_videoCache.takeFirst();
        av_frame_free(&frame);
    }

    // Clear audio and subtitle cache
    m_audioCache.clear();
    m_subtitleCache.clear();
}

bool FFmpegDecoder::openCodecContext(AVStream *&stream, AVCodecContext *&codecContext,
                                     AVMediaType type, int index)
{
    // Find stream
    int ret = 0;
    if ((ret = av_find_best_stream(m_formatContext,
                                   type, findRelativeStream(
                                       m_formatContext, index, type), -1, nullptr, 0)) < 0)
    {
        FUNC_ERROR << "Could not find stream " << av_get_media_type_string(type);
        return false;
    }

    stream = m_formatContext->streams[ret];

    // Find codec
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        FUNC_ERROR << "Cound not find codec " << av_get_media_type_string(type);
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
    ret = avcodec_parameters_to_context(codecContext, stream->codecpar);
    if (ret < 0)
    {
        FUNC_ERROR << "Failed to copy codec parameters to decoder context"
                   << av_get_media_type_string(type);
        return ret;
    }
    av_dict_set(&opts, "refcounted_frames", "0", 0);

    // Open codec and get the context
    if (avcodec_open2(codecContext, codec, &opts) < 0)
    {
        FUNC_ERROR << "Failed to open codec " << av_get_media_type_string(type);
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

/**
 * @ref ffmpeg.c line:181 : static void sub2video_copy_rect()
 */
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

static int streamCount(const AVFormatContext *format, AVMediaType type)
{
    int count = 0;

    for(size_t i = 0; i < format->nb_streams; ++i)
        if(format->streams[i]->codecpar->codec_type == type)
            ++count;

    return count;
}

static int findRelativeStream(const AVFormatContext *format,
                              int relativeIndex, AVMediaType type)
{
    int count = 0;

    for(size_t i = 0; i < format->nb_streams; ++i)
    {
        if(format->streams[i]->codecpar->codec_type == type)
        {
            if(relativeIndex == count)
                return int(i);

            ++count;
        }
    }

    return -1;
}
