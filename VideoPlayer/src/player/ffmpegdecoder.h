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
#include <QSharedPointer>
#include <QContiguousCache>

#include "ffmpeg.h"

#define VIDEO_CACHE_SIZE 128
#define AUDIO_CACHE_SIZE 256
#define SUBTITLE_CACHE_SIZE 128

#define AUDIO_DELAY 0.3
#define SUBTITLE_DEFAULT_DURATION 2

#define FUNC_ERROR qCritical() << __FUNCTION__

#define FFMPEG_ERROR(x) FUNC_ERROR << ":" << __LINE__ \
                    << ":" << av_make_error_string(m_errorBuf, sizeof (m_errorBuf), x)

typedef QPair<QSize,            // Size
              AVPixelFormat>    // Format
    VideoInfo;

struct SubtitleFrame
{
    SubtitleFrame(int width, int height) :
        image(width, height, QImage::Format_ARGB32)
    { image.fill(Qt::transparent); }

    QImage image;
    qreal start = 0;
    qreal end = 0;
};

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

    int audioTrackCount() const;

    int subtitleTrackCount() const;

    int position() const { return m_position; }

    VideoInfo videoInfo() const;

    AVFrame* takeVideoFrame();

    qint64 takeAudioData(char *data, qint64 len);

    QSharedPointer<SubtitleFrame> takeSubtitleFrame();

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

    void trackAudio(int index);
    void trackSubtitle(int index);

private slots:
    void onDecode();

protected:
    State m_state = Closed;

private:
    char m_errorBuf[AV_ERROR_MAX_STRING_SIZE];

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

    QContiguousCache<AVFrame *> m_videoCache;
    QContiguousCache<AVFrame *> m_audioCache;
    QContiguousCache<QSharedPointer<SubtitleFrame>> m_subtitleCache;

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

    bool openCodecContext(AVStream *&stream, AVCodecContext *&codecContext,
                          AVMediaType type, int index = 0);
    void closeCodecContext(AVStream *&stream, AVCodecContext *&codecContext);

    bool openSubtitleFilter(const QString &args, const QString &filterDesc);
    void closeSubtitleFilter();
};

#endif // FFMPEGDECODER_H
