/**
 * @brief FFmpeg decoder
 * @anchor Ho 229
 * @date 2021/4/13
 */

#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QUrl>
#include <QSize>
#include <QDebug>
#include <QMutex>
#include <QObject>
#include <QAudioFormat>
#include <QContiguousCache>

#include "ffmpeg.h"

#define VIDEO_CACHE_SIZE 128
#define AUDIO_CACHE_SIZE 512
#define SUBTITLE_CACHE_SIZE 64

#define FUNC_ERROR qCritical() << __FUNCTION__

#define FFMPEG_ERROR(x) FUNC_ERROR << ": line" << __LINE__ \
                    << ":" << av_make_error_string(m_errorBuf, sizeof (m_errorBuf), x)

typedef QPair<QSize,            // Size
              AVPixelFormat>    // Format
    VideoInfo;

typedef QPair<qint64,           // PTS
              QByteArray>       // PCM data
    AudioFrame;

typedef struct
{
    AVSubtitle subtitle;
    QString text;
} SubtitleFrame;

class FFmpegDecoder : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        Opened,
        Closed
    };

    explicit FFmpegDecoder(QObject *parent = nullptr);
    ~FFmpegDecoder() Q_DECL_OVERRIDE;

    bool load();
    void release();

    void setUrl(const QUrl& url){ m_url = url; }
    QUrl url() const { return m_url; }

    State state() const { return m_state; }

    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }
    bool hasSubtitle() const { return m_hasSubtitle; }

    bool hasFrame() const { return m_videoCache.count() || m_audioCache.count(); }

    bool isDecodeFinished() const { return m_isDecodeFinished; }

    bool isCacheFull() const { return m_videoCache.isFull() || m_audioCache.isFull(); }

    /**
     * @return duration of the media in seconds.
     */
    int duration() const { return m_formatContext ?
                          static_cast<int>(
                              m_formatContext->duration / AV_TIME_BASE) : 0; }

    int position() const { return m_position; }

    void seek(int position);

    VideoInfo videoInfo() const;

    AVFrame* takeVideoFrame();

    const QByteArray takeAudioData(int len);

    const QAudioFormat audioFormat() const;

    qreal fps() const;

    qreal diff() const { return m_isPtsUpdated ? m_diff : 0.0; }

    inline static qreal second(const qint64 time, const AVRational timebase)
    { return static_cast<qreal>(time) * av_q2d(timebase); }

signals:
    void callDecodec();         // Asynchronous call FFmpegDecoder::decode()
    void callSeek();            // Asynchronous call FFmpegDecoder::seek()

    void decodeFinished();

private slots:
    void decode();
    void seek();

protected:
    State m_state = Closed;

private:
    char m_errorBuf[512];

    mutable QMutex m_mutex;

    QUrl m_url;

    AVFormatContext *m_formatContext = nullptr;

    AVStream *m_videoStream = nullptr;
    AVCodecContext *m_videoCodecContext = nullptr;

    AVStream *m_audioStream = nullptr;
    AVCodecContext *m_audioCodecContext = nullptr;

    AVStream *m_subtitleStream = nullptr;
    AVCodecContext *m_subtitleCodecContext = nullptr;

    AVFilterContext *m_buffersrcContext  = nullptr;
    AVFilterContext *m_buffersinkContext = nullptr;

    SwrContext *m_swrContext = nullptr;
    SwsContext *m_swsContext = nullptr;

    QContiguousCache<AVFrame *>     m_videoCache;
    QContiguousCache<AudioFrame>    m_audioCache;
    QContiguousCache<SubtitleFrame> m_subtitleCache;

    bool m_hasVideo = false;
    bool m_hasAudio = false;
    bool m_hasSubtitle = false;

    bool m_isPtsUpdated = false;

    bool m_runnable = false;             // Is FFmpegDecoder::decode() could run

    bool m_isDecodeFinished = false;

    int m_position = 0;
    int m_targetPosition = 0;

    qint64 m_audioPts = 0;

    qreal m_diff = 0.0;

    void loadSubtitle(int index = 0);

    inline void clearCache();

    static bool openCodecContext(AVFormatContext *formatContext,
                                 AVStream **stream,
                                 AVCodecContext **codecContext,
                                 AVMediaType type, int index = 0);

    static bool initSubtitleFilter(AVFilterContext * &buffersrcContext,
                                   AVFilterContext * &buffersinkContext,
                                   const QString &args, const QString &filterDesc);
};

#endif // FFMPEGDECODER_H
