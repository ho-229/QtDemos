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
#include <QWaitCondition>
#include <QContiguousCache>

#include "ffmpeg.h"
#include "playtime.h"

#define CACHE_SIZE 128
#define ALLOW_DIFF 0.04         // 40ms

typedef QPair<QSize,            // Size
              AVPixelFormat>    // Format
    VideoInfo;

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

    void setFileName(const QUrl& url){ m_url = url; }
    QUrl fileName() const { return m_url; }

    State state() const { return m_state; }

    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }

    bool isDecodeFinished() const { return m_isDecodeFinished; }

    void resetPlayTime(){ m_startTimer.restart(); }
    void resume(){ m_isResume = true; }

    /**
     * @return duration of the media in seconds.
     */
    int duration() const { return m_formatContext ?
                          static_cast<int>(
                              m_formatContext->duration / 1000000) : 0; }

    int position() const { return m_position; }

    void seek(int position);

    bool hasVideoFrame() const { return !m_videoCache.isEmpty(); }
    AVFrame* takeVideoFrame();

    VideoInfo videoInfo() const { return m_videoCodecContext ?
                                   VideoInfo({ m_videoCodecContext->width,
                                              m_videoCodecContext->height },
                                             m_videoCodecContext->pix_fmt) :
                                   VideoInfo({}, AV_PIX_FMT_NONE); }

    bool hasAudioData() const;
    const QByteArray takeAudioData(int len);

    const QAudioFormat audioFormat() const;

    qreal fps() const { return m_hasVideo ?
                             (av_q2d(m_videoStream->avg_frame_rate))
                             : -1; }

signals:
    void callDecodec();
    void decodeFinished();

public slots:
    void decode();

protected:
    State m_state = Closed;

private:
    char m_errorBuf[512];

    mutable QMutex m_mutex;
    mutable QWaitCondition m_condition;

    QUrl m_url;

    AVFormatContext *m_formatContext = nullptr;

    AVStream *m_videoStream = nullptr;
    AVCodecContext *m_videoCodecContext = nullptr;

    AVStream *m_audioStream = nullptr;
    AVCodecContext *m_audioCodecContext = nullptr;
    SwrContext *m_swrContext = nullptr;

    QContiguousCache<AVFrame *> m_videoCache;
    QByteArray m_audioBuffer;

    PLayTime m_startTimer;

    bool m_hasVideo = false;
    bool m_hasAudio = false;

    bool m_isSeeked = false;
    bool m_isResume = false;

    bool m_isDecodeFinished = false;

    int m_position = 0;

    inline void clearCache();
    inline void printErrorString(int errnum);
    inline static bool openCodecContext(AVFormatContext *formatContext,
                                        AVStream **stream,
                                        AVCodecContext **codecContext,
                                        AVMediaType type);
};

#endif // FFMPEGDECODER_H
