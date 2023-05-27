/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2023/4/21
 */

#include "videoplayer_p.h"

#include "config.h"
#include "audiooutput.h"
#include "ffmpegdecoder.h"
#include "qglobal.h"

static inline int sigmoid(qreal value);
static inline void updateAverage(qreal &average, qreal sample);

void VideoPlayerPrivate::updateTimer()
{
    Q_Q(VideoPlayer);

    if(state != VideoPlayer::Playing)
        return;

    q->killTimer(timerId);
    q->startTimer(interval, Qt::PreciseTimer);
}

void VideoPlayerPrivate::updateAudioOutput()
{
    audioOutput->updateAudioOutput(decoder->audioFormat());

    if(state == VideoPlayer::Playing)
        audioOutput->play();
}

qint64 VideoPlayerPrivate::updateAudioData(char *data, qint64 maxlen)
{
    if(!data)
        return 0;

    qint64 free = maxlen;
    char *dest = data;
    AVFrame *frame = nullptr;

    while((frame = decoder->takeAudioFrame(free)))
    {
        const qint64 size = frame->linesize[0];
        memcpy(dest, frame->data[0], size);
        av_frame_free(&frame);

        dest += size;
        free -= size;
    }

    return maxlen - free;
}

/**
 * @note This algorithm is based on experience so the better implementation is remain
 */
void VideoPlayerPrivate::synchronize()
{
    const qreal diff = decoder->diff();
    if(!diff)
        return;

    const qreal absDiff = qAbs(diff);
    if(absDiff > ALLOW_DIFF)
    {
        // Update the timer interval to sync video to audio
        const int delta = sigmoid(diff);
        if(delta)
        {
            interval = qMin(qMax(interval - delta, 1), maxInterval);
            this->updateTimer();
        }
    }
    else if(interval != averageInterval)
    {
        interval = averageInterval;
        this->updateTimer();
    }

    updateAverage(averageInterval, interval);

    // Drop video frame if video is toooo slow, it usually works when playing high fps video
    if(diff > ALLOW_DIFF * 4)
    {
        AVFrame *frame = decoder->takeVideoFrame();
        av_frame_free(&frame);
    }
}

static inline int sigmoid(qreal value)
{
    return value * 100 / (5 + qAbs(value));
}

static inline void updateAverage(qreal &average, qreal sample)
{
    static const qreal weight = 0.97;
    average = weight * average + (1 - weight) * sample;
}
