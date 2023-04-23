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
#include <QVariant>
#include <QAudioFormat>
#include <QSharedPointer>
#include <QContiguousCache>

#include <ffmpeg.h>

#define FUNC_ERROR qCritical() << __FUNCTION__

struct SubtitleFrame
{
    SubtitleFrame(int width, int height) :
        image(width, height, QImage::Format_ARGB32)
    { image.fill(Qt::transparent); }

    QImage image;
    qreal start = 0;
    qreal end = 0;
};

class FFmpegDecoder final : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        Opened,
        Closed
    };
    Q_ENUM(State)

    explicit FFmpegDecoder(QObject *parent = nullptr);
    ~FFmpegDecoder() Q_DECL_OVERRIDE;

    void requestInterrupt() { m_runnable = false; }

    void setUrl(const QUrl& url) { m_url = url; }
    QUrl url() const { return m_url; }

    State state() const { return m_state; }

    QString errorString() const { return m_errorBuf; }

    int activeVideoTrack() const;
    int activeAudioTrack() const;
    int activeSubtitleTrack() const;

    int videoTrackCount() const;
    int audioTrackCount() const;
    int subtitleTrackCount() const;

    bool hasFrame() const { return m_videoCache.count() || m_audioCache.count(); }

    bool seekable() const;

    bool isEnd() const { return m_isEnd; }
    bool isCacheFull() const { return m_videoCache.isFull() || m_audioCache.isFull(); }
    bool isBitmapSubtitleActived() const;

    /**
     * @return duration of the media in seconds.
     */
    int duration() const;
    int position() const;

    QSize videoSize() const;
    AVPixelFormat videoPixelFormat() const;

    const QAudioFormat audioFormat() const;

    AVFrame *takeVideoFrame();
    qint64 takeAudioData(char *data, qint64 len);
    QSharedPointer<SubtitleFrame> takeSubtitleFrame();

    qreal fps() const;

    qreal diff() const;

    inline static qreal second(const qint64 time, const AVRational timebase)
    { return static_cast<qreal>(time) * av_q2d(timebase); }

signals:
    void stateChanged(FFmpegDecoder::State);
    void positionChanged(int);

    void activeVideoTrackChanged(int);
    void activeAudioTrackChanged(int);
    void activeSubtitleTrackChanged(int);

public slots:
    void load();
    void release();

    void seek(int position);

    void setActiveVideoTrack(int index);
    void setActiveAudioTrack(int index);
    void setActiveSubtitleTrack(int index);

    void decode();

private:
    void decodeVideo();
    void decodeAudio();
    void decodeSubtitle(AVPacket *packet);

    void clearCache();

    bool openCodecContext(AVStream *&stream, AVCodecContext *&codecContext,
                          AVMediaType type, int index = 0);
    void closeCodecContext(AVStream *&stream, AVCodecContext *&codecContext);

    bool openSubtitleFilter(const QString &args, const QString &filterDesc);
    void closeSubtitleFilter();

private:
    State m_state = Closed;

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

    qreal m_fps = qQNaN();

    volatile bool m_isDecoding = false;
    volatile bool m_runnable = false;             // Is FFmpegDecoder::decode() could run
    volatile bool m_isEnd = false;

    volatile mutable int m_position = 0;
    volatile qreal m_videoTime = 0.0;
    volatile qreal m_audioTime = 0.0;

    QList<int> m_videoIndexes;
    QList<int> m_audioIndexes;
    QList<QVariant> m_subtitleIndexes;
    int m_subtitleIndex = -1;
};

#endif // FFMPEGDECODER_H
