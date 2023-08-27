/**
 * @brief Video Player Private
 * @anchor Ho 229
 * @date 2023/4/21
 */

#include "videoplayer_p.h"

#include "audiooutput.h"
#include "ffmpegdecoder.h"
#include "videorenderer.h"

void VideoPlayerPrivate::updateTimer(int newInterval)
{
    Q_Q(VideoPlayer);

    interval = newInterval;
    q->killTimer(timerId);
    q->startTimer(interval, Qt::PreciseTimer);
}

void VideoPlayerPrivate::restartAudioOutput()
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

    while(free)
    {
        if(!audioFrame)
        {
            if(!(audioFrame = decoder->takeAudioFrame()))
                break;
        }

        if(!audioClock.isValid() || (audioOutput->isLowDataLeft() && dest == data))
            audioClock.update(FFmpegDecoder::framePts(audioFrame));

        const auto size = qMin(qint64(audioFrame->linesize[0]) - audioFramePos, free);

        memcpy(dest, audioFrame->data[0] + audioFramePos, size);
        dest += size;
        free -= size;
        audioFramePos += size;

        if(audioFramePos >= audioFrame->linesize[0])
        {
            audioFramePos = 0;
            av_frame_free(&audioFrame);
        }
    }

    return maxlen - free;
}

void VideoPlayerPrivate::updateVideoFrame()
{
    AVFrame *frame = nullptr;
    while((frame = decoder->takeVideoFrame()))
    {
        const qreal pts = FFmpegDecoder::framePts(frame);
        if(!qFuzzyCompare(pts, -1))
        {
            videoClock.update(pts);
            auto nextInterval = FFmpegDecoder::frameDuration(frame);

            if(audioClock.isValid())
                nextInterval -= audioClock.time() - videoClock.time()/* - audioOutput->bufferDuration()*/;
            nextInterval *= 1000;

            if(nextInterval < 1)
            {
                av_frame_free(&frame);
                continue;
            }

            if(interval != nextInterval)
                this->updateTimer(nextInterval);
        }

        videoRenderer->updateVideoFrame(frame);
        break;
    }
}

void VideoPlayerPrivate::updateSubtitleFrame()
{
    SubtitleFrame *frame = nullptr;
    if((frame = decoder->takeSubtitleFrame(videoClock.time())))
        videoRenderer->updateSubtitleFrame(frame);
}
