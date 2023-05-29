/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2023/4/21
 */

#include "videoplayer_p.h"

#include "audiooutput.h"
#include "ffmpegdecoder.h"
#include "videorenderer.h"

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
        if(!audioClock.isValid() || (audioOutput->isLowDataLeft() && dest == data))
            audioClock.update(FFmpegDecoder::framePts(frame));

        memcpy(dest, frame->data[0], size);
        av_frame_free(&frame);

        dest += size;
        free -= size;
    }

    return maxlen - free;
}

void VideoPlayerPrivate::updateVideoFrame()
{
    AVFrame *frame = nullptr;
    while((frame = decoder->takeVideoFrame()))
    {
        videoClock.update(FFmpegDecoder::framePts(frame));
        auto nextInterval = FFmpegDecoder::frameDuration(frame);

        if(audioClock.isValid())
            nextInterval -= audioClock.time() - videoClock.time();
        nextInterval *= 1000;

        if(nextInterval < 1)
        {
            av_frame_free(&frame);
            continue;
        }

        if(interval != nextInterval)
        {
            interval = nextInterval;
            this->updateTimer();
        }

        videoRenderer->updateVideoFrame(frame);
        // FIXME: bitmap subtitle
//        videoRenderer->updateSubtitleFrame(decoder->takeSubtitleFrame());
        break;
    }
}
