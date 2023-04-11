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
#include <QImage>
#include <QObject>
#include <QAudioFormat>
#include <QContiguousCache>

#include "ffmpeg.h"

#define VIDEO_CACHE_SIZE 128
#define AUDIO_CACHE_SIZE 256
#define SUBTITLE_CACHE_SIZE 128
#define AUDIO_DELAY 0.3

#define FUNC_ERROR qCritical() << __FUNCTION__

#define FFMPEG_ERROR(x) FUNC_ERROR << ": line" << __LINE__ \
                    << ":" << av_make_error_string(m_errorBuf, sizeof (m_errorBuf), x)

typedef QPair<QSize,            // Size
              AVPixelFormat>    // Format
    VideoInfo;

struct SubtitleFrame
{
    QImage image;
    qreal pts = -1;      // In second
};

Q_DECLARE_METATYPE(SubtitleFrame)

class FFmpegDecoder : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        Error,      // only used by stateChanged signal
        Opened,
        Closed
    };
    Q_ENUM(State)

    enum SubtitleType
    {
        None,
        TextBased,
        Bitmap
    };
    Q_ENUM(SubtitleType)

    explicit FFmpegDecoder(QObject *parent = nullptr);
    ~FFmpegDecoder() Q_DECL_OVERRIDE;

    void requestInterrupt() { m_runnable = false; }

    void setUrl(const QUrl& url) { m_url = url; }
    QUrl url() const { return m_url; }

    State state() const { return m_state; }

    QString errorString() const { return m_errorBuf; }

    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }

    SubtitleType subtitleType() const { return m_subtitleType; }

    bool hasFrame() const { return m_videoCache.count() || m_audioCache.count(); }

    bool seekable() const;

    bool isEnd() const { return m_isEnd; }

    bool isCacheFull() const { return m_videoCache.isFull() || m_audioCache.isFull(); }

    /**
     * @return duration of the media in seconds.
     */
    int duration() const;

    void trackAudio(int index);
    int audioTrackCount() const { return m_formatContext ? streamCount(
            m_formatContext, AVMEDIA_TYPE_AUDIO) : -1; }

    void trackSubtitle(int index);
    int subtitleTrackCount() const;

    int position() const { return m_position; }

    VideoInfo videoInfo() const;

    QSize subtitleSize() const { return m_subtitleCodecContext ?
                                      QSize(m_subtitleCodecContext->width,
                                            m_subtitleCodecContext->height) : QSize(); }

    AVFrame* takeVideoFrame();

    qint64 takeAudioData(char *data, qint64 len);

    SubtitleFrame takeSubtitleFrame();

    const QAudioFormat audioFormat() const;

    qreal fps() const;

    qreal diff() const { return m_isPtsUpdated ? m_diff : 0.0; }

    inline static qreal second(const qint64 time, const AVRational timebase)
    { return static_cast<qreal>(time) * av_q2d(timebase); }

signals:
    void seeked();
    void stateChanged(FFmpegDecoder::State);

public slots:
    void load();
    void release();

    void seek(int position);

private slots:
    void onDecode();

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
    QContiguousCache<AVFrame *>     m_audioCache;
    QContiguousCache<SubtitleFrame> m_subtitleCache;

    SubtitleType m_subtitleType = None;

    bool m_hasVideo = false;
    bool m_hasAudio = false;

    volatile bool m_isPtsUpdated = false;
    volatile bool m_runnable = false;             // Is FFmpegDecoder::decode() could run
    volatile bool m_isEnd = false;

    volatile qreal m_diff = 0.0;
    volatile int m_position = 0;
    qreal m_videoTime = 0.0;

    volatile qint64 m_audioPts = 0;

    void loadSubtitle(int index = 1);

    void clearCache();

    static inline void releaseContext(AVCodecContext *&context)
    {
        avcodec_flush_buffers(context);
        avcodec_free_context(&context);
    }

    static inline void releaseFilter(AVFilterContext *&context)
    {
        avfilter_free(context);
        context = nullptr;
    }

    static bool openCodecContext(AVFormatContext *formatContext,
                                 AVStream **stream,
                                 AVCodecContext **codecContext,
                                 AVMediaType type, int index = 0);

    static bool initSubtitleFilter(AVFilterContext * &buffersrcContext,
                                   AVFilterContext * &buffersinkContext,
                                   const QString &args, const QString &filterDesc);

    /**
     * @ref ffmpeg.c line:181 : static void sub2video_copy_rect()
     */
    static void mergeSubtitle(uint8_t *dst, int dst_linesize, int w, int h,
                              AVSubtitleRect *r);

    static QImage loadFromAVFrame(const AVFrame *frame);

    static int streamCount(const AVFormatContext *format, AVMediaType type);

    static int findRelativeStream(const AVFormatContext *format, int relativeIndex,
                                  AVMediaType type);
};

#endif // FFMPEGDECODER_H
