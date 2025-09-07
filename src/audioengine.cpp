#include "audioengine.h"
#include <QDebug>
#include <QFileInfo>
#include <QMutexLocker>
#include <QUrl>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
    , m_mediaPlayer(nullptr)
    , m_audioOutput(nullptr)
    , m_positionTimer(nullptr)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_currentPosition(0)
    , m_duration(0)
    , m_volume(1.0f)
    , m_muted(false)
    , m_sampleRate(44100)
{
    initializeAudio();
    setupConnections();
}

AudioEngine::~AudioEngine()
{
    if (m_positionTimer) {
        m_positionTimer->stop();
    }
    
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
}

void AudioEngine::initializeAudio()
{
    try {
        // Initialize Qt Multimedia components
        m_audioOutput = new QAudioOutput(this);
        m_mediaPlayer = new QMediaPlayer(this);
        
        if (!m_audioOutput || !m_mediaPlayer) {
            qDebug() << "AudioEngine: Failed to create audio components";
            return;
        }
        
        m_mediaPlayer->setAudioOutput(m_audioOutput);
        
        // Set initial volume
        m_audioOutput->setVolume(m_volume);
        
        // Create position update timer for smooth timeline sync
        m_positionTimer = new QTimer(this);
        m_positionTimer->setInterval(POSITION_UPDATE_INTERVAL_MS);
        
        qDebug() << "AudioEngine: Creating timer connection...";
        bool connected = connect(m_positionTimer, &QTimer::timeout, this, &AudioEngine::updatePosition);
        qDebug() << "AudioEngine: Timer connection successful:" << connected;
        
        // Test the timer connection immediately
        connect(m_positionTimer, &QTimer::timeout, []() {
            qDebug() << "*** TIMER TIMEOUT SIGNAL RECEIVED ***";
        });
        
        qDebug() << "AudioEngine: Audio system initialized successfully";
    } catch (const std::exception& e) {
        qDebug() << "AudioEngine: Exception during initialization:" << e.what();
    } catch (...) {
        qDebug() << "AudioEngine: Unknown exception during initialization";
    }
}

void AudioEngine::setupConnections()
{
    // Connect media player signals
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, 
            this, &AudioEngine::handleMediaPlayerPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &AudioEngine::handleMediaPlayerStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, &AudioEngine::handleMediaPlayerError);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
            this, [this](qint64 duration) {
                QMutexLocker locker(&m_mutex);
                m_duration = duration;
                emit durationChanged(duration);
            });
}

void AudioEngine::play()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "=== AudioEngine::play() DEBUG ===";
    qDebug() << "m_mediaPlayer exists:" << (m_mediaPlayer != nullptr);
    if (m_mediaPlayer) {
        qDebug() << "m_mediaPlayer->hasAudio():" << m_mediaPlayer->hasAudio();
        qDebug() << "m_mediaPlayer->source():" << m_mediaPlayer->source();
        qDebug() << "m_mediaPlayer->mediaStatus():" << m_mediaPlayer->mediaStatus();
        qDebug() << "m_mediaPlayer->playbackState():" << m_mediaPlayer->playbackState();
    }
    qDebug() << "=================================";
    
    if (m_mediaPlayer && m_mediaPlayer->hasAudio()) {
        m_mediaPlayer->play();
        m_isPlaying = true;
        m_isPaused = false;
        m_positionTimer->start();
        
        qDebug() << "AudioEngine: Position timer started with interval:" << m_positionTimer->interval() << "ms";
        qDebug() << "AudioEngine: Position timer isActive:" << m_positionTimer->isActive();
        
        // Force a manual timer check
        QTimer::singleShot(100, [this]() {
            qDebug() << "Manual timer check - isActive:" << m_positionTimer->isActive();
            qDebug() << "Manual timer check - interval:" << m_positionTimer->interval();
            qDebug() << "Manual timer check - forcing updatePosition call...";
            updatePosition();
        });
        
        emit playbackStateChanged(true);
        qDebug() << "AudioEngine: Started playback";
    } else {
        qDebug() << "AudioEngine: No audio loaded for playback";
        if (m_mediaPlayer && !m_mediaPlayer->hasAudio()) {
            qDebug() << "AudioEngine: Media player exists but hasAudio() returned false";
            qDebug() << "AudioEngine: Trying to play anyway...";
            m_mediaPlayer->play();
            m_isPlaying = true;
            m_isPaused = false;
            m_positionTimer->start();
            
            qDebug() << "AudioEngine: Position timer started (fallback) with interval:" << m_positionTimer->interval() << "ms";
            qDebug() << "AudioEngine: Position timer isActive (fallback):" << m_positionTimer->isActive();
            
            emit playbackStateChanged(true);
        } else {
            emit audioError(AudioError::FileNotFound, "No audio file loaded");
        }
    }
}

void AudioEngine::stop()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        m_mediaPlayer->setPosition(0);
        m_currentPosition = 0;
        m_isPlaying = false;
        m_isPaused = false;
        m_positionTimer->stop();
        
        emit playbackStateChanged(false);
        emit positionChanged(0.0);
        qDebug() << "AudioEngine: Stopped playback and returned to start";
    }
}

void AudioEngine::pause()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_mediaPlayer && m_isPlaying) {
        m_mediaPlayer->pause();
        m_isPlaying = false;
        m_isPaused = true;
        m_positionTimer->stop();
        
        emit playbackStateChanged(false);
        qDebug() << "AudioEngine: Paused playback";
    }
}

void AudioEngine::setPosition(qint64 positionMs)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_mediaPlayer && m_mediaPlayer->hasAudio()) {
        m_mediaPlayer->setPosition(positionMs);
        m_currentPosition = positionMs;
        
        double seconds = msToSeconds(positionMs);
        emit positionChanged(seconds);
        qDebug() << "AudioEngine: Set position to" << positionMs << "ms (" << seconds << "s)";
    }
}

qint64 AudioEngine::position() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentPosition;
}

qint64 AudioEngine::duration() const
{
    QMutexLocker locker(&m_mutex);
    return m_duration;
}

bool AudioEngine::isPlaying() const
{
    QMutexLocker locker(&m_mutex);
    return m_isPlaying;
}

bool AudioEngine::isPaused() const
{
    QMutexLocker locker(&m_mutex);
    return m_isPaused;
}

AudioResult AudioEngine::loadAudioFile(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString errorMsg = QString("Audio file not found: %1").arg(filePath);
        emit audioError(AudioError::FileNotFound, errorMsg);
        return AudioResult(AudioError::FileNotFound, errorMsg);
    }
    
    // Check if media player is properly initialized
    if (!m_mediaPlayer || !m_audioOutput) {
        QString errorMsg = "Audio system not properly initialized";
        emit audioError(AudioError::DeviceError, errorMsg);
        return AudioResult(AudioError::DeviceError, errorMsg);
    }
    
    // Stop current playback
    m_mediaPlayer->stop();
    m_isPlaying = false;
    m_isPaused = false;
    if (m_positionTimer && m_positionTimer->isActive()) {
        m_positionTimer->stop();
    }
    
    // Load new audio file
    QUrl audioUrl = QUrl::fromLocalFile(filePath);
    qDebug() << "AudioEngine: Loading audio file:" << audioUrl.toString();
    
    try {
        m_mediaPlayer->setSource(audioUrl);
        
        // Force immediate buffer completion for local files
        qDebug() << "AudioEngine: Forcing buffer completion for local file...";
        m_mediaPlayer->setPosition(0);
        
        // Reset position
        m_currentPosition = 0;
        
        emit audioLoaded(filePath);
        qDebug() << "AudioEngine: Successfully loaded audio file:" << filePath;
        
        return AudioResult(); // Success
    } catch (const std::exception& e) {
        QString errorMsg = QString("Failed to load audio file: %1").arg(e.what());
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    } catch (...) {
        QString errorMsg = "Unknown error occurred while loading audio file";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
}

void AudioEngine::clearAudio()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        m_mediaPlayer->setSource(QUrl());
        m_isPlaying = false;
        m_isPaused = false;
        m_currentPosition = 0;
        m_duration = 0;
        m_positionTimer->stop();
        
        emit playbackStateChanged(false);
        emit positionChanged(0.0);
        qDebug() << "AudioEngine: Cleared audio";
    }
}

void AudioEngine::setTimelinePosition(double seconds)
{
    setPosition(secondsToMs(seconds));
}

double AudioEngine::getTimelinePosition() const
{
    return msToSeconds(position());
}

void AudioEngine::setVolume(float volume)
{
    QMutexLocker locker(&m_mutex);
    
    m_volume = qBound(0.0f, volume, 1.0f);
    if (m_audioOutput) {
        m_audioOutput->setVolume(m_volume);
    }
    qDebug() << "AudioEngine: Set volume to" << m_volume;
}

float AudioEngine::getVolume() const
{
    QMutexLocker locker(&m_mutex);
    return m_volume;
}

void AudioEngine::setMuted(bool muted)
{
    QMutexLocker locker(&m_mutex);
    
    m_muted = muted;
    if (m_audioOutput) {
        m_audioOutput->setMuted(muted);
    }
    qDebug() << "AudioEngine: Set muted to" << muted;
}

bool AudioEngine::isMuted() const
{
    QMutexLocker locker(&m_mutex);
    return m_muted;
}

int AudioEngine::getSampleRate() const
{
    QMutexLocker locker(&m_mutex);
    return m_sampleRate;
}

void AudioEngine::setSampleRate(int sampleRate)
{
    QMutexLocker locker(&m_mutex);
    m_sampleRate = sampleRate;
    qDebug() << "AudioEngine: Set sample rate to" << sampleRate;
}

// Transport control slots
void AudioEngine::onTransportPlay()
{
    if (isPaused()) {
        play(); // Resume from pause
    } else if (!isPlaying()) {
        play(); // Start playback
    }
}

void AudioEngine::onTransportStop()
{
    pause(); // Pause without returning to start
}

void AudioEngine::onTransportPause()
{
    pause();
}

void AudioEngine::onTransportStopAndReturn()
{
    stop(); // Stop and return to start
}

void AudioEngine::onPositionChanged(double seconds)
{
    setTimelinePosition(seconds);
}

// Private slots
void AudioEngine::handleMediaPlayerPositionChanged(qint64 position)
{
    // Use tryLock to prevent blocking
    if (m_mutex.tryLock()) {
        m_currentPosition = position;
        m_mutex.unlock();
    }
    // Don't emit here - let updatePosition() handle it for consistent timing
}

void AudioEngine::handleMediaPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    // Use tryLock to prevent blocking during state changes
    if (!m_mutex.tryLock()) {
        // Queue this state change for later processing
        QTimer::singleShot(10, this, [this, state]() {
            handleMediaPlayerStateChanged(state);
        });
        return;
    }
    
    bool wasPlaying = m_isPlaying;
    
    switch (state) {
    case QMediaPlayer::PlayingState:
        m_isPlaying = true;
        m_isPaused = false;
        if (m_positionTimer && !m_positionTimer->isActive()) {
            m_positionTimer->start();
        }
        break;
    case QMediaPlayer::PausedState:
        m_isPlaying = false;
        m_isPaused = true;
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        break;
    case QMediaPlayer::StoppedState:
        m_isPlaying = false;
        m_isPaused = false;
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        break;
    }
    
    // Unlock before emitting signals
    m_mutex.unlock();
    
    // Only emit if state actually changed
    if (wasPlaying != m_isPlaying) {
        emit playbackStateChanged(m_isPlaying);
    }
    
    qDebug() << "AudioEngine: Playback state changed to" << state;
}

void AudioEngine::handleMediaPlayerError(QMediaPlayer::Error error, const QString& errorString)
{
    // Use tryLock for error handling to prevent deadlocks
    if (m_mutex.tryLock()) {
        m_isPlaying = false;
        m_isPaused = false;
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        m_mutex.unlock();
    }
    
    AudioError audioError;
    switch (error) {
    case QMediaPlayer::ResourceError:
        audioError = AudioError::FileNotFound;
        break;
    case QMediaPlayer::FormatError:
        audioError = AudioError::UnsupportedFormat;
        break;
    default:
        audioError = AudioError::DecodingFailed;
        break;
    }
    
    // Emit signals without holding mutex
    emit this->audioError(audioError, errorString);
    emit playbackStateChanged(false);
    qDebug() << "AudioEngine: Media player error:" << errorString;
}

void AudioEngine::updatePosition()
{
    // Don't use mutex in timer callback - just read atomic values
    if (!m_isPlaying || !m_mediaPlayer) {
        return;
    }
    
    qDebug() << "=== AudioEngine::updatePosition() CALLED (no mutex) ===";
    
    // Get position without locking - Qt objects are thread-safe for reading
    qint64 currentPos = m_mediaPlayer->position();
    double seconds = msToSeconds(currentPos);
    
    qDebug() << "AudioEngine::updatePosition() - Position:" << currentPos << "ms (" << seconds << "seconds)";
    
    // Emit position change immediately without mutex
    emit positionChanged(seconds);
    
    qDebug() << "=== AudioEngine::updatePosition() END ===";
}

// Utility functions
double AudioEngine::msToSeconds(qint64 ms) const
{
    return static_cast<double>(ms) / 1000.0;
}

qint64 AudioEngine::secondsToMs(double seconds) const
{
    return static_cast<qint64>(seconds * 1000.0);
}
