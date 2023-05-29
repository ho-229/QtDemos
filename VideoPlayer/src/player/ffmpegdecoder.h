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
        image(width, height, QImage::Format_RGBA8888)
    { image.fill(Qt::transparent); }

    QImage image;
    qreal start = 0;
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

    /**
     * @brief Request the interruption of the FFmpegDecoer::decode
     */
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

    /**
     * @return duration of the media in seconds.
     */
    int duration() const;

    const QAudioFormat audioFormat() const;

    AVFrame *takeVideoFrame();
    AVFrame *takeAudioFrame(qint64 maxlen);
    SubtitleFrame *takeSubtitleFrame(qreal time);

    /**
     * @return qQNaN() if not available(eg. no video frames or only a single frame like album cover),
     *         otherwise returns frame rate of video stream
     */
    qreal fps() const;

    static qreal framePts(const AVFrame *frame);
    static qreal frameDuration(const AVFrame *frame);

signals:
    void seeked();
    void stateChanged(FFmpegDecoder::State);

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
    void decodeVideo(AVPacket *packet);
    void decodeAudio(AVPacket *packet);
    void decodeSubtitle(AVPacket *packet);

    bool shouldDecode() const;

    void clearCache();

    bool openCodecContext(AVStream *&stream, AVCodecContext *&codecContext,
                          AVMediaType type, int index);
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

    AVFilterGraph *m_filterGraph = nullptr;
    AVFilterContext *m_buffersrcContext  = nullptr;
    AVFilterContext *m_buffersinkContext = nullptr;

    SwrContext *m_swrContext = nullptr;
    SwsContext *m_swsContext = nullptr;

    QContiguousCache<AVFrame *> m_videoCache;
    QContiguousCache<AVFrame *> m_audioCache;
    QContiguousCache<SubtitleFrame *> m_subtitleCache;

    qreal m_fps = qQNaN();                          // See also FFmpegDecoder::fps()

    volatile bool m_isDecoding = false;
    volatile bool m_runnable = false;               // Is FFmpegDecoder::decode() could run
    volatile bool m_isEnd = false;

    int m_seekTarget = -1;                          // -1 means undefined

    QList<int> m_videoIndexes;
    QList<int> m_audioIndexes;
    QList<QVariant> m_subtitleIndexes;
    int m_subtitleIndex = -1;
};

#endif // FFMPEGDECODER_H
