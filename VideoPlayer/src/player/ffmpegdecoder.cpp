﻿/**
 * @brief FFmpeg decoder
 * @anchor Ho 229
 * @date 2021/4/13
 */

#include "ffmpegdecoder.h"

FFmpegDecoder::FFmpegDecoder(QObject *parent) :
    QObject(parent)
{
    //avformat_network_init();
    av_log_set_level(AV_LOG_INFO);

    m_videoCache.setCapacity(CACHE_SIZE);

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
    m_hasVideo = openCodecContext(m_formatContext, &m_videoStream,
                                  &m_videoCodecContext, AVMEDIA_TYPE_VIDEO);

    // Initialize audio codec context
    if((m_hasAudio = openCodecContext(m_formatContext, &m_audioStream,
                                       &m_audioCodecContext, AVMEDIA_TYPE_AUDIO)))
    {
        if(m_audioCodecContext->sample_fmt != AV_SAMPLE_FMT_S16)
        {
            m_swrContext = swr_alloc_set_opts(m_swrContext,
                                              av_get_default_channel_layout(2),
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
    m_run = true;

    emit callDecodec();                 // Asynchronous call FFmpegDecoder::decode()

    return true;
}

void FFmpegDecoder::release()
{
    if(m_state == Closed)
        return;

    m_state = Closed;
    m_run = false;

    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    avcodec_free_context(&m_videoCodecContext);
    avcodec_free_context(&m_audioCodecContext);

    swr_close(m_swrContext);

    avformat_close_input(&m_formatContext);

    m_position = 0;
    m_targetPosition = 0;

    m_videoStream = nullptr;
    m_audioStream = nullptr;

    m_videoStream = nullptr;
    m_audioStream = nullptr;

    m_videoCodecContext = nullptr;
    m_audioCodecContext = nullptr;
}

void FFmpegDecoder::seek(int position)
{
    if(m_state == Closed || position == this->position())
        return;

    m_targetPosition = position;
    m_run = false;

    emit callSeek();
}

void FFmpegDecoder::seek()
{
    m_mutex.lock();
    this->clearCache();
    m_mutex.unlock();

    av_seek_frame(m_formatContext, m_videoStream->index, static_cast<qint64>
                  (m_targetPosition / av_q2d(m_videoStream->time_base)),
                  /*AVSEEK_FLAG_BACKWARD |*/ AVSEEK_FLAG_FRAME);

    avcodec_flush_buffers(m_videoCodecContext);
    avcodec_flush_buffers(m_audioCodecContext);

    m_isSeeked = true;
    m_run = true;

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

    if(m_isSeeked)
    {
        m_startTimer.synchronize(static_cast<qint64>(
            static_cast<qreal>(m_videoCache.first()->pts)
            * av_q2d(m_videoStream->time_base) * 1000));

        m_isSeeked = false;
    }

    // Update the system clock when resume
    if(m_isResume)
    {
        m_startTimer.synchronize(static_cast<qint64>(
            static_cast<qreal>(m_videoCache.first()->pts)
            * av_q2d(m_videoStream->time_base) * 1000));

        m_isResume = false;
    }

    // Synchronize the video clock to the system clock
    qreal diff = 0;
    do
    {
        diff = static_cast<qreal>(m_startTimer.elapsed()) / 1000
               - static_cast<qreal>(m_videoCache.first()->pts)
                     * av_q2d(m_videoStream->time_base);

        if(diff > ALLOW_DIFF)         // Too slow
        {
            AVFrame *frame = m_videoCache.takeFirst();
            av_frame_free(&frame);
        }
        else if(diff < -ALLOW_DIFF)   // Too quick
        {
            AVFrame *frame = m_videoCache.first();
            AVFrame *newFrame = av_frame_alloc();

            av_frame_copy(newFrame, frame);
            newFrame->pts -= qint64(diff / 2);
            m_videoCache.prepend(newFrame);
        }
    }
    while(!(diff < ALLOW_DIFF && diff > -ALLOW_DIFF));

    AVFrame *frame = m_videoCache.takeFirst();

    m_position = static_cast<int>(static_cast<double>(frame->pts)
                                  * av_q2d(m_videoStream->time_base));
    m_mutex.unlock();

    if(m_videoCache.count() <= CACHE_SIZE / 2)
        emit callDecodec();

    return frame;
}

const QByteArray FFmpegDecoder::takeAudioData(int len)
{
    if(m_state == Closed || m_audioBuffer.isEmpty())
        return QByteArray();

    m_mutex.lock();
    const QByteArray ret = m_audioBuffer.remove(0, len);
    m_mutex.unlock();

    if(m_audioBuffer.size() < 8192)
        emit callDecodec();

    return ret;
}

const QAudioFormat FFmpegDecoder::audioFormat() const
{
    QAudioFormat format;

    if(m_state == Opened && m_hasAudio)
    {
        format.setSampleRate(m_audioCodecContext->sample_rate);
        format.setChannelCount(av_get_channel_layout_nb_channels(
            m_audioCodecContext->channel_layout));
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::UnSignedInt);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
    }

    return format;
}

void FFmpegDecoder::decode()
{
    AVPacket* packet = nullptr;

    while(m_state == Opened && !m_videoCache.isFull() && m_run
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

            if(!avcodec_receive_frame(m_videoCodecContext, frame))
            {
                m_mutex.lock();
                m_videoCache.append(frame);
                m_mutex.unlock();
            }
            else
                av_frame_free(&frame);
        }

        // Audio frames decode
        else if(packet->stream_index == m_audioStream->index &&
                 !avcodec_send_packet(m_audioCodecContext, packet))
        {
            AVFrame *frame = av_frame_alloc();
            while(!avcodec_receive_frame(m_audioCodecContext, frame))
            {
                const int size =
                    av_samples_get_buffer_size(nullptr,
                                               m_audioCodecContext->channels,
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
                    m_audioBuffer.append(pcm);
                    m_mutex.unlock();
                }
                else
                {
                    m_mutex.lock();
                    m_audioBuffer.append(reinterpret_cast<char *>(frame->data[0]),
                                         size);
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
    if(!m_audioBuffer.isEmpty())
        m_audioBuffer.clear();
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
