#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>

#if HAVE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}
#endif
#include <QList>
#include <QMutex>
#include "audioerror.h"

class AudioTrack;

class AudioEngine : public QObject
{
    Q_OBJECT

public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    // Playback control
    void play();
    void stop();
    void pause();
    void setPosition(qint64 positionMs);
    qint64 position() const;
    qint64 duration() const;
    
    // Transport state
    bool isPlaying() const;
    bool isPaused() const;
    
    // Audio file management
    AudioResult loadAudioFile(const QString& filePath);
    void clearAudio();
    
    // Timeline synchronization
    void setTimelinePosition(double seconds);
    double getTimelinePosition() const;
    
    // Audio settings
    void setVolume(float volume); // 0.0 to 1.0
    float getVolume() const;
    void setMuted(bool muted);
    bool isMuted() const;
    
    // Sample rate and timing
    int getSampleRate() const;
    void setSampleRate(int sampleRate);

public slots:
    void onTransportPlay();
    void onTransportStop();
    void onTransportPause();
    void onTransportStopAndReturn();
    void onPositionChanged(double seconds);

signals:
    void positionChanged(double seconds);
    void playbackStateChanged(bool isPlaying);
    void durationChanged(qint64 duration);
    void audioLoaded(const QString& filePath);
    void audioError(AudioError error, const QString& message);

private slots:
    void handleMediaPlayerPositionChanged(qint64 position);
    void handleMediaPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void handleMediaPlayerError(QMediaPlayer::Error error, const QString& errorString);
    void updatePosition();

private:
    void initializeAudio();
    void setupConnections();
    double msToSeconds(qint64 ms) const;
    qint64 secondsToMs(double seconds) const;

    // Legacy Qt Multimedia (to be removed)
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;
    QAudioSink* m_audioSink;
    QIODevice* m_audioBuffer;
    QTimer* m_positionTimer;
    
#if HAVE_FFMPEG
    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    SwrContext* m_swrContext;
    int m_audioStreamIndex;
#endif
    
    // Audio state
    bool m_isPlaying;
    bool m_isPaused;
    qint64 m_currentPosition; // in milliseconds
    qint64 m_duration; // in milliseconds
    float m_volume;
    bool m_muted;
    int m_sampleRate;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Constants
    static const int POSITION_UPDATE_INTERVAL_MS = 16; // 60fps updates
};

#endif // AUDIOENGINE_H
