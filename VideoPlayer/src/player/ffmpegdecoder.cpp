/**
 * @brief FFmpeg decoder
 * @anchor Ho 229
 * @date 2021/4/13
 */

#include "ffmpegdecoder.h"

#include <QThread>

FFmpegDecoder::FFmpegDecoder(QObject *parent) :
    QObject(parent)
{
    //avformat_network_init();
    av_log_set_level(AV_LOG_INFO);

    m_videoCache.setCapacity(VIDEO_CACHE_SIZE);
    m_audioCache.setCapacity(AUDIO_CACHE_SIZE);

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
    if((ret = avformat_open_input(
             &m_formatContext, m_url.toLocalFile().toLocal8Bit().data(),
             nullptr, nullptr)) < 0)
    {
        this->printErrorString(ret);
        return false;
    }

    // Find stream info
    if((ret = avformat_find_stream_info(m_formatContext, nullptr)) < 0)
    {
        this->printErrorString(ret);
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
            m_swrContext = swr_alloc_set_opts(m_swrContext,
                                              av_get_default_channel_layout(
                                                  2),
                                              AV_SAMPLE_FMT_S16,
                                              m_audioCodecContext->sample_rate,
                                              av_get_default_channel_layout(
                                                  m_audioCodecContext->channels),
                                              m_audioCodecContext->sample_fmt,
                                              m_audioCodecContext->sample_rate,
                                              0, nullptr);
            swr_init(m_swrContext);
        }
    }

    if(!(m_hasVideo || m_hasAudio))     // If there is no video and audio
        return false;

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

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    // Wait for finished
    while(m_isRunning)
        QThread::currentThread()->msleep(50);

    avcodec_free_context(&m_videoCodecContext);
    avcodec_free_context(&m_audioCodecContext);

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

    m_videoStream = nullptr;
    m_audioStream = nullptr;

    m_videoCodecContext = nullptr;
    m_audioCodecContext = nullptr;
}

void FFmpegDecoder::seek(int position)
{
    if(m_state == Closed || position == this->position() ||
        (m_isSeeked && position == m_targetPosition))
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
    IsRunning running(m_isRunning);

    av_seek_frame(m_formatContext, m_videoStream->index, static_cast<qint64>
                  (m_targetPosition / av_q2d(m_videoStream->time_base)),
                  /*AVSEEK_FLAG_BACKWARD |*/ AVSEEK_FLAG_FRAME);

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    avcodec_flush_buffers(m_videoCodecContext);
    avcodec_flush_buffers(m_audioCodecContext);

    m_isSeeked = true;
    m_runnable = true;

    this->decode();
}

AVFrame *FFmpegDecoder::takeVideoFrame()
{
    m_mutex.lock();

    if(m_state == Closed || m_videoCache.isEmpty())
    {
        m_mutex.unlock();
        return nullptr;
    }

    // Synchronize the video clock to the audio clock if has audio
    qreal diff = 0;
    while(!m_isSeeked && m_hasAudio && m_videoCache.count() > 1)
    {
        diff = second(m_audioPts, m_audioStream->time_base)
               - second(m_videoCache.first()->pts, m_videoStream->time_base);

        if(diff > ALLOW_DIFF)         // Too slow
        {
            AVFrame *frame = m_videoCache.takeFirst();
            av_frame_free(&frame);
        }
        else if(diff < -ALLOW_DIFF)   // Too quick
        {
            m_mutex.unlock();
            return nullptr;
        }
        else
            break;
    }

    AVFrame *frame = m_videoCache.takeFirst();

    m_position = static_cast<int>(second(frame->pts, m_videoStream->time_base));

    m_mutex.unlock();

    if(m_videoCache.count() <= VIDEO_CACHE_SIZE / 2)
        emit callDecodec();

    return frame;
}

const QByteArray FFmpegDecoder::takeAudioData(int len)
{
    m_mutex.lock();

    if(m_state == Closed || m_audioCache.isEmpty() || !len)
    {
        m_mutex.unlock();
        return QByteArray();
    }

    if(m_isSeeked)
        m_isSeeked = false;

    int free = len;
    QByteArray ret;

    while(!m_audioCache.isEmpty() && m_audioCache.first().second.size() < free)
    {
        const AudioFrame frame = m_audioCache.takeFirst();

        m_audioPts = frame.first;
        ret.append(frame.second);

        free -= frame.second.size();
    }
    m_mutex.unlock();

    if(m_audioCache.size() < AUDIO_CACHE_SIZE / 2)
        emit callDecodec();

    return ret;
}

const QAudioFormat FFmpegDecoder::audioFormat() const
{
    QAudioFormat format;

    if(m_state == Opened && m_hasAudio)
    {
        format.setSampleRate(m_audioCodecContext->sample_rate);
        format.setChannelCount(2);
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
    }

    return format;
}

void FFmpegDecoder::decode()
{
    AVPacket* packet = nullptr;
    IsRunning running(m_isRunning);

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
        if(packet->stream_index == m_videoStream->index &&
            !avcodec_send_packet(m_videoCodecContext, packet))
        {
            AVFrame *frame = av_frame_alloc();
            if(frame && !avcodec_receive_frame(m_videoCodecContext, frame))
            {
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

        // Audio frames decode
        else if(packet->stream_index == m_audioStream->index &&
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

    // Clear audio cache
    if(!m_audioCache.isEmpty())
        m_audioCache.clear();
}

void FFmpegDecoder::printErrorString(int errnum)
{
    qCritical() << "FFmpeg: "
                << av_make_error_string(m_errorBuf,
                                        sizeof (m_errorBuf), errnum);
}

bool FFmpegDecoder::openCodecContext(AVFormatContext *formatContext, AVStream **stream,
                                     AVCodecContext **codecContext, AVMediaType type)
{
    // Find stream
    int ret = 0;
    if ((ret = av_find_best_stream(formatContext,
                                   type, -1, -1, nullptr, 0)) < 0)
    {
        qCritical() << "Could not find stream " << av_get_media_type_string(type);
        return false;
    }
    *stream = formatContext->streams[ret];

    // Find codec
    AVCodec *codec = avcodec_find_decoder((*stream)->codecpar->codec_id);
    if (!codec)
    {
        qCritical() << "Cound not find codec " << av_get_media_type_string(type);
        return false;
    }

    // Allocate codec context
    if(!(*codecContext = avcodec_alloc_context3(codec)))
    {
        qCritical() << "Failed to allocate codec context";
        return false;
    }

    (*codecContext)->thread_count = 1;

    AVDictionary *opts = nullptr;
    ret = avcodec_parameters_to_context(*codecContext, (*stream)->codecpar);
    if (ret < 0)
    {
        qWarning() << "Failed to copy codec parameters to decoder context"
                   << av_get_media_type_string(type);
        return ret;
    }
    av_dict_set(&opts, "refcounted_frames", "0", 0);

    // Open codec and get the context
    if (avcodec_open2(*codecContext, codec, &opts) < 0)
    {
        qCritical() << "Failed to open codec " << av_get_media_type_string(type);
        return false;
    }

    return true;
}
