/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#include "audiooutput.h"
#include "ffmpegdecoder.h"

#include <QAudioOutput>

AudioOutput::AudioOutput(FFmpegDecoder *decoder, QObject *parent) :
    QObject(parent)
{
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
        FUNC_ERROR << "Could not initialize SDL2:" << SDL_GetError();

    if(!decoder)
        qCritical() << __FUNCTION__ << ": Decoder is not valid";

    m_userData.decoder = decoder;
}

void AudioOutput::setAudioFormat(const SDL_AudioSpec format)
{
    m_audioBuffer = format;

    if(!m_audioBuffer.format)
        FUNC_ERROR << ": This format is not support";
}

void AudioOutput::setVolume(qreal volume)
{
    m_userData.volume = int(volume * SDL_MIX_MAXVOLUME);
}

qreal AudioOutput::volume() const
{
    return m_userData.volume / SDL_MIX_MAXVOLUME;
}

void AudioOutput::play()
{
    m_audioBuffer.userdata = &m_userData;
    m_audioBuffer.callback = fillBuffer;

    if(SDL_OpenAudio(&m_audioBuffer, nullptr) < 0)
    {
        FUNC_ERROR << ":Can not open audio";
        return;
    }

    SDL_PauseAudio(false);
}

void AudioOutput::pause()
{
    SDL_PauseAudio(true);
}

void AudioOutput::resume()
{
    SDL_PauseAudio(false);
}

void AudioOutput::stop()
{
    SDL_PauseAudio(true);
    SDL_CloseAudio();
}

void AudioOutput::fillBuffer(void *userdata, Uint8 *stream, int len)
{
    if(!len)
        return;

    SDL_memset(stream, 0, size_t(len));
    AudioUserData *user = reinterpret_cast<AudioUserData *>(userdata);

    const QByteArray data(user->decoder->takeAudioData(len));

    if(data.isEmpty())
        return;

    SDL_MixAudio(stream, reinterpret_cast<const uint8_t *>(data.data()),
                 static_cast<Uint32>(data.size()), user->volume);
}
