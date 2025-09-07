#ifndef FFMPEGAUDIOENGINE_H
#define FFMPEGAUDIOENGINE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QBuffer>
#include <QByteArray>

// Forward declaration
class AudioIODevice;

#if HAVE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}
#endif

#include "audioerror.h"

class FFmpegAudioEngine : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegAudioEngine(QObject *parent = nullptr);
    ~FFmpegAudioEngine();

    // Playback control
    void play();
    void stop();
    void pause();
    
    // State queries
    bool isPlaying() const;
    bool isPaused() const;
    
    // Audio file management
    AudioResult loadAudioFile(const QString& filePath);
    void clearAudio();
    
    // Position control
    void setTimelinePosition(double seconds);
    double getCurrentPosition() const;
    double getDuration() const;
    
    // Volume control
    void setVolume(float volume);
    float getVolume() const;
    void setMuted(bool muted);
    bool isMuted() const;

signals:
    void positionChanged(double seconds);
    void playbackStateChanged(bool isPlaying);
    void durationChanged(double seconds);
    void audioLoaded(const QString& filePath);
    void audioError(AudioError error, const QString& message);

public slots:
    void onTransportPlay();
    void onTransportStop();
    void onTransportPause();
    void onTransportStopAndReturn();
    void onPositionChanged(double seconds);

private slots:
    void updatePosition();
    void onPlaybackComplete(); // Called when audio playback finishes
    void onAudioStateChanged(QAudio::State state); // Called when QAudioSink state changes

private:
    void initializeAudio();
    bool initializeFFmpeg();
    void cleanupFFmpeg();
    AudioResult decodeAudioFile(const QString& filePath);
    void setupAudioOutput();
    
    // FFmpeg components
#if HAVE_FFMPEG
    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    SwrContext* m_swrContext;
    int m_audioStreamIndex;
    AVFrame* m_frame;
    AVPacket* m_packet;
#endif
    
    // Qt Audio components
    QAudioSink* m_audioSink;
    AudioIODevice* m_audioDevice; // Custom hardware-driven device
    QBuffer* m_audioBuffer;
    QByteArray m_audioData;
    QAudioFormat m_audioFormat;
    
    // Timing and position
    QTimer* m_positionTimer;
    // Removed m_playbackTimer - using hardware-driven callbacks instead
    
    // Audio state
    bool m_isPlaying;
    bool m_isPaused;
    qint64 m_currentPosition; // in milliseconds
    qint64 m_duration; // in milliseconds
    qint64 m_playbackStartTime;
    float m_volume;
    bool m_muted;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Constants
    static constexpr int POSITION_UPDATE_INTERVAL_MS = 16; // 60fps updates
    static constexpr int BUFFER_SIZE = 1024; // Smaller buffer to reduce crackling
};

#endif // FFMPEGAUDIOENGINE_H
