/**
 * @brief FFmpeg decoder
 * @anchor Ho 229
 * @date 2021/4/13
 */

#include "ffmpegdecoder.h"
#include "qaudioformat.h"

#include <QDir>
#include <QThread>
#include <QFileInfo>
#include <QMutexLocker>

FFmpegDecoder::FFmpegDecoder(QObject *parent) :
    QObject(parent)
{
    //avformat_network_init();
    av_log_set_level(AV_LOG_INFO);

    m_videoCache.setCapacity(VIDEO_CACHE_SIZE);
    m_audioCache.setCapacity(AUDIO_CACHE_SIZE);
    m_subtitleCache.setCapacity(SUBTITLE_CACHE_SIZE);

    QObject::connect(this, &FFmpegDecoder::callDecodec, this,
                     &FFmpegDecoder::decode);
    QObject::connect(this, &FFmpegDecoder::callSeek, this,
                     QOverload<>::of(&FFmpegDecoder::seek));
}

FFmpegDecoder::~FFmpegDecoder()
{
    this->release();
    //avformat_network_deinit();
}

bool FFmpegDecoder::load()
{
    this->release();        // Reset

    int ret = 0;

    // Open file
    // Note that FFmpeg accepts filename encoded in UTF-8
    if((ret = avformat_open_input(&m_formatContext, m_url.toLocalFile().toUtf8().data(),
             nullptr, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        return false;
    }

    // Find stream info
    if((ret = avformat_find_stream_info(m_formatContext, nullptr)) < 0)
    {
        FFMPEG_ERROR(ret);
        return false;
    }

    // Print file infomation
    av_dump_format(m_formatContext, 0, m_formatContext->url, 0);

    // Initialize video codec context
    if((m_hasVideo = openCodecContext(m_formatContext, &m_videoStream,
                                       &m_videoCodecContext, AVMEDIA_TYPE_VIDEO)))
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
    if((m_hasAudio = openCodecContext(m_formatContext, &m_audioStream,
                                       &m_audioCodecContext, AVMEDIA_TYPE_AUDIO)))
    {
        if(m_audioCodecContext->sample_fmt != AV_SAMPLE_FMT_S16)
        {
            AVChannelLayout dest;
            av_channel_layout_default(&dest, 2);
            swr_alloc_set_opts2(&m_swrContext, &dest,
                                AV_SAMPLE_FMT_S16,
                                m_audioCodecContext->sample_rate,
                                &m_audioCodecContext->ch_layout,
                                m_audioCodecContext->sample_fmt,
                                m_audioCodecContext->sample_rate,
                                0, nullptr);
            swr_init(m_swrContext);
        }
    }

    if(!(m_hasVideo || m_hasAudio))     // If there is no video and audio
        return false;

    this->loadSubtitle();

    this->thread()->start();            // Start the decode thread

    m_isDecodeFinished = false;
    m_state = Opened;
    m_runnable = true;

    emit callDecodec();                 // Asynchronous call FFmpegDecoder::decode()

    return true;
}

void FFmpegDecoder::release()
{
    if(m_state == Closed)
        return;

    m_state = Closed;
    m_runnable = false;

    this->thread()->quit();             // Tell the decode thread exit

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    // Wait for finished
    if(!this->thread()->wait())
        FUNC_ERROR << ": Decode thread exit failed";

    if(m_hasVideo)
        releaseContext(m_videoCodecContext);

    if(m_hasAudio)
        releaseContext(m_audioCodecContext);

    if(m_subtitleCodecContext)
        releaseContext(m_subtitleCodecContext);

    if(m_buffersrcContext)
        releaseFilter(m_buffersrcContext);

    if(m_buffersinkContext)
        releaseFilter(m_buffersinkContext);

    if(m_swrContext)
    {
        swr_close(m_swrContext);
        m_swrContext = nullptr;
    }

    if(m_swsContext)
    {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    avformat_close_input(&m_formatContext);

    m_position = 0;
    m_targetPosition = 0;

    m_isPtsUpdated = false;

    m_videoStream = nullptr;
    m_audioStream = nullptr;
    m_subtitleStream = nullptr;

    m_hasAudio = false;
    m_hasVideo = false;
    m_hasSubtitle = false;
}

void FFmpegDecoder::trackedAudio(int index)
{
    if(index < 0 || index > this->audioTrackCount() || index == m_audioStream->index)
        return;

    m_mutex.lock();

    this->clearCache();
    releaseContext(m_audioCodecContext);
    openCodecContext(m_formatContext, &m_audioStream, &m_audioCodecContext,
                     AVMEDIA_TYPE_AUDIO, index);

    m_mutex.unlock();

    emit callDecodec();
}

void FFmpegDecoder::trackSubtitle(int index)
{
    if(index < 0 || index > this->subtitleTrackCount())
        return;

    m_mutex.lock();
    this->clearCache();

    if(m_subtitleCodecContext)
    {
        releaseContext(m_subtitleCodecContext);
        openCodecContext(m_formatContext, &m_subtitleStream, &m_subtitleCodecContext,
                         AVMEDIA_TYPE_SUBTITLE, index);
    }
    else
    {
        if(m_buffersrcContext && m_buffersinkContext)
        {
            releaseFilter(m_buffersrcContext);
            releaseFilter(m_buffersinkContext);
        }

        this->loadSubtitle(index);
    }

    m_mutex.unlock();

    emit callDecodec();
}

int FFmpegDecoder::subtitleTrackCount() const
{
    if(m_state == Closed)
        return 0;

    int ret = 0;

    if((ret = streamCount(m_formatContext, AVMEDIA_TYPE_SUBTITLE)) > 0)
        return ret;

    const QFileInfo fileInfo(m_url.toLocalFile());

    const QDir subtitleDir(fileInfo.absoluteDir());

    const QStringList subtitleList =
        subtitleDir.entryList({{"*.ass"}, {"*.srt"}, {"*.lrc"}},
                              QDir::Files).filter(fileInfo.baseName());

    return subtitleList.size();
}

void FFmpegDecoder::seek(int position)
{
    if(m_state == Closed || position == m_position)
        return;

    m_targetPosition = position;
    m_runnable = false;

    emit callSeek();
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

void FFmpegDecoder::seek()
{
    if(m_state == Closed || m_position == m_targetPosition)
        return;

    const AVStream *seekStream = m_hasVideo ? m_videoStream : m_audioStream;
    av_seek_frame(m_formatContext, seekStream->index, static_cast<qint64>
                  (m_targetPosition / av_q2d(seekStream->time_base)),
                  AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);

    m_position = m_targetPosition;

    // Clear frame cache
    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    // Flush the codec buffers
    if(m_videoCodecContext)
        avcodec_flush_buffers(m_videoCodecContext);
    if(m_audioCodecContext)
        avcodec_flush_buffers(m_audioCodecContext);

    m_isPtsUpdated = false;
    m_runnable = true;

    this->decode();
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

    auto toFFmpegFormat = [](QString &fileName) -> QString& {
        fileName.replace('/', "\\\\");
        return fileName.insert(fileName.indexOf(":\\"), char('\\'));
    };

    if(av_find_best_stream(m_formatContext, AVMEDIA_TYPE_SUBTITLE,
                            findRelativeStream(m_formatContext, index,
                              AVMEDIA_TYPE_SUBTITLE), -1, nullptr, 0) > 0)
    {
        QString subtitleFileName = m_url.toLocalFile();

        if((m_hasSubtitle = initSubtitleFilter(m_buffersrcContext,
                                                m_buffersinkContext, args,
                                                makeFilterDesc(toFFmpegFormat(
                                                    subtitleFileName), index))))
            return;

        // If is not text based subtitles
        m_hasSubtitle = openCodecContext(m_formatContext, &m_subtitleStream,
                                         &m_subtitleCodecContext,
                                         AVMEDIA_TYPE_SUBTITLE, index);
    }

    // If no found subtitle stream in video file
    else
    {
        const QFileInfo fileInfo(m_url.toLocalFile());

        const QDir subtitleDir(fileInfo.absoluteDir());

        const QStringList subtitleList =
            subtitleDir.entryList({{"*.ass"}, {"*.srt"}, {"*.lrc"}},
                                  QDir::Files).filter(fileInfo.baseName());

        if(subtitleList.size() <= index - 1)    // Out of range
            return;

        QString subtitleFileName = subtitleDir.absolutePath()
                                   + '/' + subtitleList.at(index - 1);

        if(QFileInfo::exists(subtitleFileName))
        {
            m_hasSubtitle = initSubtitleFilter(
                m_buffersrcContext, m_buffersinkContext, args,
                makeFilterDesc(toFFmpegFormat(subtitleFileName), 0));
        }
    }
}

AVFrame *FFmpegDecoder::takeVideoFrame()
{
    m_mutex.lock();

    if(m_state == Closed || m_videoCache.isEmpty())
    {
        m_mutex.unlock();
        return nullptr;
    }

    AVFrame *frame = m_videoCache.takeFirst();

    m_videoTime = second(frame->pts, m_videoStream->time_base);

    if(m_isPtsUpdated)
        m_diff = second(m_audioPts, m_audioStream->time_base) - m_videoTime - AUDIO_DELAY;

    m_position = static_cast<int>(m_videoTime);

    m_mutex.unlock();

    if(m_videoCache.count() <= VIDEO_CACHE_SIZE / 2 && !m_isDecodeFinished)
        emit callDecodec();

    return frame;
}

qint64 FFmpegDecoder::takeAudioData(char *data, qint64 len)
{
    m_mutex.lock();
    if(m_state == Closed || m_audioCache.isEmpty() || !len)
    {
        m_mutex.unlock();
        return {};
    }

    qint64 free = len;
    char *dest = data;

    while(!m_audioCache.isEmpty() && m_audioCache.first().second.size() <= free)
    {
        const AudioFrame &&frame = m_audioCache.takeFirst();
        const auto size = frame.second.size();

        m_audioPts = frame.first;
        memcpy(dest, frame.second.data(), size);

        free -= size;
        dest += size;
    }

    m_mutex.unlock();
    m_isPtsUpdated = true;

    if(m_audioCache.size() < AUDIO_CACHE_SIZE / 2 && !m_isDecodeFinished)
        emit callDecodec();

    return len - free;
}

SubtitleFrame FFmpegDecoder::takeSubtitleFrame()
{
    QMutexLocker locker(&m_mutex);

    if(!m_subtitleCache.isEmpty() && m_subtitleCache.first().pts < m_videoTime)
        m_subtitleCache.removeFirst();

    if(m_subtitleCache.isEmpty() ||
        m_subtitleCache.first().pts < m_videoTime ||
        m_subtitleCache.first().pts - m_videoTime > 0.04)
        return {};

    return m_subtitleCache.takeFirst();
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

void FFmpegDecoder::decode()
{
    AVPacket* packet = nullptr;

    while(m_state == Opened && !this->isCacheFull() && m_runnable
           && (packet = av_packet_alloc()))
    {
        if(av_read_frame(m_formatContext, packet))      // Read frame
        {
            av_packet_free(&packet);

            m_isDecodeFinished = true;
            emit decodeFinished();
            break;
        }

        // Video frame decode
        if(m_hasVideo && packet->stream_index == m_videoStream->index &&
            !avcodec_send_packet(m_videoCodecContext, packet))
        {
            AVFrame *frame = av_frame_alloc();
            AVFrame *filterFrame = av_frame_alloc();

            if(frame && !avcodec_receive_frame(m_videoCodecContext, frame))
            {
                m_mutex.lock();
                if(m_buffersrcContext && m_buffersinkContext)
                {
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
                m_mutex.unlock();

                if(m_swsContext)
                {
                    AVFrame *swsFrame = av_frame_alloc();

                    swsFrame->pts = frame->pts;
                    swsFrame->width = m_videoCodecContext->width;
                    swsFrame->height = m_videoCodecContext->height;
                    swsFrame->format = AV_PIX_FMT_YUV420P;

                    av_frame_get_buffer(swsFrame, 0);

                    sws_scale(m_swsContext, frame->data, frame->linesize, 0,
                              m_videoCodecContext->height, swsFrame->data,
                              swsFrame->linesize);

                    av_frame_free(&frame);

                    m_mutex.lock();
                    m_videoCache.append(swsFrame);
                    m_mutex.unlock();
                }
                else
                {
                    m_mutex.lock();
                    m_videoCache.append(frame);
                    m_mutex.unlock();
                }
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
                const int size =
                    av_samples_get_buffer_size(nullptr,
                                               2,
                                               frame->nb_samples,
                                               AV_SAMPLE_FMT_S16,
                                               0);

                if(m_swrContext)
                {
                    QByteArray pcm;
                    pcm.resize(size);
                    uint8_t *outData[2] = { nullptr };
                    outData[0] = reinterpret_cast<uint8_t *>(pcm.data());

                    swr_convert(m_swrContext, outData, frame->nb_samples,
                                reinterpret_cast<const uint8_t **>(frame),
                                frame->nb_samples);

                    m_mutex.lock();
                    m_audioCache.append({frame->pts, pcm});
                    m_mutex.unlock();
                }
                else
                {
                    m_mutex.lock();
                    m_audioCache.append({frame->pts, {reinterpret_cast<const char *>
                                                      (frame->data[0]), size}});
                    m_mutex.unlock();
                }
            }

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
                AVFrame *frame = av_frame_alloc();

                frame->width = m_subtitleCodecContext->width;
                frame->height = m_subtitleCodecContext->height;
                frame->format = AV_PIX_FMT_RGB32;

                av_frame_get_buffer(frame, 0);

                for(uint i = 0; i < subtitle.num_rects; ++i)
                    mergeSubtitle(frame->data[0], frame->linesize[0],
                                  frame->width, frame->height, subtitle.rects[i]);

                SubtitleFrame subtitleFrame;
                subtitleFrame.image = loadFromAVFrame(frame);
                subtitleFrame.pts = second(packet->pts, m_subtitleStream->time_base);

                m_mutex.lock();
                m_subtitleCache.append(subtitleFrame);
                m_mutex.unlock();

                avsubtitle_free(&subtitle);
                av_frame_free(&frame);
            }
        }

        av_packet_free(&packet);
    }
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

bool FFmpegDecoder::openCodecContext(AVFormatContext *formatContext, AVStream **stream,
                                     AVCodecContext **codecContext, AVMediaType type,
                                     int index)
{
    // Find stream
    int ret = 0;
    if ((ret = av_find_best_stream(formatContext,
                                   type, findRelativeStream(
                                       formatContext, index, type), -1, nullptr, 0)) < 0)
    {
        FUNC_ERROR << "Could not find stream" << av_get_media_type_string(type);
        return false;
    }

    *stream = formatContext->streams[ret];

    // Find codec
    const AVCodec *codec = avcodec_find_decoder((*stream)->codecpar->codec_id);
    if (!codec)
    {
        FUNC_ERROR << "Cound not find codec " << av_get_media_type_string(type);
        return false;
    }

    // Allocate codec context
    if(!(*codecContext = avcodec_alloc_context3(codec)))
    {
        FUNC_ERROR << "Failed to allocate codec context";
        return false;
    }

    (*codecContext)->thread_count = 1;

    AVDictionary *opts = nullptr;
    ret = avcodec_parameters_to_context(*codecContext, (*stream)->codecpar);
    if (ret < 0)
    {
        FUNC_ERROR << "Failed to copy codec parameters to decoder context"
                   << av_get_media_type_string(type);
        return ret;
    }
    av_dict_set(&opts, "refcounted_frames", "0", 0);

    // Open codec and get the context
    if (avcodec_open2(*codecContext, codec, &opts) < 0)
    {
        FUNC_ERROR << "Failed to open codec " << av_get_media_type_string(type);
        return false;
    }

    return true;
}

bool FFmpegDecoder::initSubtitleFilter(AVFilterContext *&buffersrcContext,
                                       AVFilterContext *&buffersinkContext,
                                       const QString& args, const QString& filterDesc)
{
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    AVFilterInOut *output = avfilter_inout_alloc();
    AVFilterInOut *input = avfilter_inout_alloc();
    AVFilterGraph *filterGraph = avfilter_graph_alloc();

    auto release = [&output, &input] {
        avfilter_inout_free(&output);
        avfilter_inout_free(&input);
    };

    if (!output || !input || !filterGraph)
    {
        release();
        return false;
    }

    // Create in filter using "arg"
    if (avfilter_graph_create_filter(&buffersrcContext, buffersrc, "in",
                                     args.toLocal8Bit().data(), nullptr, filterGraph) < 0)
    {
        FUNC_ERROR << "Has Error: line =" << __LINE__;
        release();

        buffersrcContext = nullptr;
        buffersinkContext = nullptr;

        return false;
    }

    // Create out filter
    if (avfilter_graph_create_filter(&buffersinkContext, buffersink, "out",
                                     nullptr, nullptr, filterGraph) < 0)
    {
        FUNC_ERROR << "Has Error: line =" << __LINE__;
        release();

        buffersrcContext = nullptr;
        buffersinkContext = nullptr;

        return false;
    }

    output->name = av_strdup("in");
    output->next = nullptr;
    output->pad_idx = 0;
    output->filter_ctx = buffersrcContext;

    input->name = av_strdup("out");
    input->next = nullptr;
    input->pad_idx = 0;
    input->filter_ctx = buffersinkContext;

    if(avfilter_graph_parse_ptr(filterGraph, filterDesc.toLocal8Bit().data(),
                                 &input, &output, nullptr) < 0)
    {
        FUNC_ERROR << "Has Error: line =" << __LINE__;
        release();

        buffersrcContext = nullptr;
        buffersinkContext = nullptr;

        return false;
    }

    if(avfilter_graph_config(filterGraph, nullptr) < 0)
    {
        FUNC_ERROR << "Has Error: line =" << __LINE__;
        release();

        buffersrcContext = nullptr;
        buffersinkContext = nullptr;

        return false;
    }

    release();
    return true;
}

void FFmpegDecoder::mergeSubtitle(uint8_t *dst, int dst_linesize, int w, int h,
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

QImage FFmpegDecoder::loadFromAVFrame(const AVFrame *frame)
{
    QImage image(frame->width, frame->height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    for(int y = 0; y < frame->height; ++y)
        memcpy(image.scanLine(y), frame->data[0] + y * frame->linesize[0],
               size_t(frame->width * 3));

    return image;
}

int FFmpegDecoder::streamCount(const AVFormatContext *format, AVMediaType type)
{
    int count = 0;

    for(size_t i = 0; i < format->nb_streams; ++i)
        if(format->streams[i]->codecpar->codec_type == type)
            ++count;

    return count;
}

int FFmpegDecoder::findRelativeStream(const AVFormatContext *format,
                                      int relativeIndex, AVMediaType type)
{
    int count = 0;

    if(relativeIndex >= 1)
        for(size_t i = 0; i < format->nb_streams; ++i)
            if(format->streams[i]->codecpar->codec_type == type)
                if(relativeIndex == ++count)
                    return int(i);

    return -1;
}
