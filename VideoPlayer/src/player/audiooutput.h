/**
 * @brief Audio Output
 * @anchor Ho 229
 * @date 2021/5/1
 */

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

extern "C"
{
#include <SDL.h>
}

#include <QObject>

class FFmpegDecoder;

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(FFmpegDecoder * decoder, QObject *parent = nullptr);

    void setAudioFormat(const SDL_AudioSpec format);

    void setVolume(qreal volume);
    qreal volume() const;

    void play();
    void pause();
    void resume();
    void stop();

signals:

private:
    struct AudioUserData
    {
        FFmpegDecoder *decoder = nullptr;
        int volume = SDL_MIX_MAXVOLUME;
    } m_userData;

    SDL_AudioSpec m_audioBuffer;

    static void SDLCALL fillBuffer(void *userdata, Uint8 *stream,
                                   int len);
};

#endif // AUDIOOUTPUT_H
