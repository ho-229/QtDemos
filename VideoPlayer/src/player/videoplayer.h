/**
 * @brief Video Player
 * @anchor Ho 229
 * @date 2021/4/14
 */

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QQuickFramebufferObject>

class VideoPlayerPrivate;

class VideoPlayer : public QQuickFramebufferObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE play NOTIFY playingChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE pause NOTIFY pausedChanged)

    // Read only property
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)

public:
    VideoPlayer(QQuickItem *parent = nullptr);
    virtual ~VideoPlayer() Q_DECL_OVERRIDE;

    Renderer *createRenderer() const Q_DECL_OVERRIDE;

    void setSource(const QUrl& source);
    QUrl source() const;

    void play(bool playing = true);
    bool isPlaying() const;

    void pause(bool paused = true);
    bool isPaused() const;

    /**
     * @return duration of the media in seconds.
     */
    int duration() const;

    int position() const;

    Q_INVOKABLE void seek(int position);

signals:
    void sourceChanged(QUrl source);
    void playingChanged(bool playing);
    void pausedChanged(bool paused);
    void durationChanged(int duration);
    void positionChanged(int position);

private:
    VideoPlayerPrivate * const d_ptr;

    Q_DECLARE_PRIVATE(VideoPlayer)

    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

};

#endif // VIDEOPLAYER_H
