#include "ffmpegaudioengine.h"
#include "audioiodevice.h"
#include <QDebug>
#include <QFileInfo>
#include <QMutexLocker>
#include <QThread>

// Static constants are now defined inline in header with constexpr

FFmpegAudioEngine::FFmpegAudioEngine(QObject *parent)
    : QObject(parent)
    , m_audioSink(nullptr)
    , m_audioDevice(nullptr)
    , m_audioBuffer(nullptr)
    , m_positionTimer(nullptr)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_currentPosition(0)
    , m_duration(0)
    , m_playbackStartTime(0)
    , m_volume(1.0f)
    , m_muted(false)
#if HAVE_FFMPEG
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_swrContext(nullptr)
    , m_audioStreamIndex(-1)
    , m_frame(nullptr)
    , m_packet(nullptr)
#endif
{
    initializeAudio();
}

FFmpegAudioEngine::~FFmpegAudioEngine()
{
    // Stop all timers and audio immediately
    if (m_positionTimer) {
        m_positionTimer->stop();
        m_positionTimer->deleteLater();
    }
    
    // Stop audio sink immediately to prevent delays
    if (m_audioSink) {
        m_audioSink->stop();
        m_audioSink->deleteLater();
    }
    
    // Clean up audio device
    if (m_audioDevice) {
        m_audioDevice->close();
        m_audioDevice->deleteLater();
    }
    
    cleanupFFmpeg();
}

void FFmpegAudioEngine::initializeAudio()
{
    qDebug() << "FFmpegAudioEngine: Initializing audio system...";
    
    try {
        // Initialize FFmpeg
        if (!initializeFFmpeg()) {
            qDebug() << "FFmpegAudioEngine: Failed to initialize FFmpeg";
            return;
        }
        
        // Setup audio format - will be updated when loading audio file
        m_audioFormat.setSampleRate(44100); // Default, will be overridden
        m_audioFormat.setChannelCount(2);
        m_audioFormat.setSampleFormat(QAudioFormat::Int16);
        
        // Create audio buffer
        m_audioBuffer = new QBuffer(&m_audioData, this);
        
        // Create position update timer
        m_positionTimer = new QTimer(this);
        m_positionTimer->setInterval(POSITION_UPDATE_INTERVAL_MS);
        connect(m_positionTimer, &QTimer::timeout, this, &FFmpegAudioEngine::updatePosition);
        
        // Removed m_playbackTimer - using hardware-driven callbacks instead
        
        qDebug() << "FFmpegAudioEngine: Audio system initialized successfully";
    } catch (const std::exception& e) {
        qDebug() << "FFmpegAudioEngine: Exception during initialization:" << e.what();
    } catch (...) {
        qDebug() << "FFmpegAudioEngine: Unknown exception during initialization";
    }
}

bool FFmpegAudioEngine::initializeFFmpeg()
{
#if HAVE_FFMPEG
    qDebug() << "FFmpegAudioEngine: Initializing FFmpeg libraries...";
    
    // Initialize FFmpeg (only needed for older versions)
    // av_register_all(); // Deprecated in newer FFmpeg versions
    
    return true;
#else
    qDebug() << "FFmpegAudioEngine: FFmpeg not available";
    return false;
#endif
}

void FFmpegAudioEngine::cleanupFFmpeg()
{
#if HAVE_FFMPEG
    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }
    
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
    
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }
    
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
#endif
}

AudioResult FFmpegAudioEngine::loadAudioFile(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "FFmpegAudioEngine: Loading audio file:" << filePath;
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString errorMsg = QString("Audio file not found: %1").arg(filePath);
        emit audioError(AudioError::FileNotFound, errorMsg);
        return AudioResult(AudioError::FileNotFound, errorMsg);
    }
    
    // Stop current playback
    if (m_isPlaying) {
        stop();
    }
    
    // Clean up previous audio data
    cleanupFFmpeg();
    m_audioData.clear();
    
    // Decode the audio file
    AudioResult result = decodeAudioFile(filePath);
    if (!result.isSuccess()) {
        return result;
    }
    
    // Setup audio output
    setupAudioOutput();
    
    emit audioLoaded(filePath);
    qDebug() << "FFmpegAudioEngine: Successfully loaded audio file:" << filePath;
    
    return AudioResult(); // Success
}

AudioResult FFmpegAudioEngine::decodeAudioFile(const QString& filePath)
{
#if HAVE_FFMPEG
    qDebug() << "FFmpegAudioEngine: Decoding audio file with FFmpeg...";
    
    // Open input file
    if (avformat_open_input(&m_formatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
        QString errorMsg = "Could not open audio file";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
    
    // Retrieve stream information
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        QString errorMsg = "Could not find stream information";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
    
    // Find audio stream
    m_audioStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            break;
        }
    }
    
    if (m_audioStreamIndex == -1) {
        QString errorMsg = "Could not find audio stream";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
    
    // Get codec parameters
    AVCodecParameters* codecpar = m_formatContext->streams[m_audioStreamIndex]->codecpar;
    
    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        QString errorMsg = "Unsupported codec";
        emit audioError(AudioError::UnsupportedFormat, errorMsg);
        return AudioResult(AudioError::UnsupportedFormat, errorMsg);
    }
    
    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        QString errorMsg = "Could not allocate codec context";
        emit audioError(AudioError::MemoryError, errorMsg);
        return AudioResult(AudioError::MemoryError, errorMsg);
    }
    
    // Copy codec parameters to context
    if (avcodec_parameters_to_context(m_codecContext, codecpar) < 0) {
        QString errorMsg = "Could not copy codec parameters";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
    
    // Open codec
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        QString errorMsg = "Could not open codec";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
    
    // Setup resampler for consistent output format (newer FFmpeg API)
    // Use the original file's sample rate instead of forcing 44100 to prevent speed issues
    int outputSampleRate = m_codecContext->sample_rate;
    qDebug() << "FFmpegAudioEngine: Input sample rate:" << m_codecContext->sample_rate << "Hz";
    qDebug() << "FFmpegAudioEngine: Output sample rate:" << outputSampleRate << "Hz";
    
    AVChannelLayout stereo_layout = AV_CHANNEL_LAYOUT_STEREO;
    int ret = swr_alloc_set_opts2(&m_swrContext,
                                  &stereo_layout, AV_SAMPLE_FMT_S16, outputSampleRate,
                                  &m_codecContext->ch_layout, m_codecContext->sample_fmt, m_codecContext->sample_rate,
                                  0, nullptr);
    
    if (ret < 0 || !m_swrContext || swr_init(m_swrContext) < 0) {
        QString errorMsg = "Could not initialize resampler";
        emit audioError(AudioError::DecodingFailed, errorMsg);
        return AudioResult(AudioError::DecodingFailed, errorMsg);
    }
    
    // Update audio format to match the loaded file
    m_audioFormat.setSampleRate(outputSampleRate);
    qDebug() << "FFmpegAudioEngine: Updated QAudioFormat sample rate to" << outputSampleRate << "Hz";
    
    // Calculate duration
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_duration = m_formatContext->duration / AV_TIME_BASE * 1000; // Convert to milliseconds
        emit durationChanged(m_duration / 1000.0);
    }
    
    // Allocate frame and packet
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    
    if (!m_frame || !m_packet) {
        QString errorMsg = "Could not allocate frame or packet";
        emit audioError(AudioError::MemoryError, errorMsg);
        return AudioResult(AudioError::MemoryError, errorMsg);
    }
    
    // Decode entire file into memory for simplicity
    qDebug() << "FFmpegAudioEngine: Decoding audio data...";
    
    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        if (m_packet->stream_index == m_audioStreamIndex) {
            // Send packet to decoder
            if (avcodec_send_packet(m_codecContext, m_packet) >= 0) {
                // Receive frames from decoder
                while (avcodec_receive_frame(m_codecContext, m_frame) >= 0) {
                    // Resample to our target format
                    uint8_t* output_buffer;
                    int output_samples = swr_get_out_samples(m_swrContext, m_frame->nb_samples);
                    av_samples_alloc(&output_buffer, nullptr, 2, output_samples, AV_SAMPLE_FMT_S16, 0);
                    
                    int converted_samples = swr_convert(m_swrContext, &output_buffer, output_samples,
                                                       (const uint8_t**)m_frame->data, m_frame->nb_samples);
                    
                    if (converted_samples > 0) {
                        int buffer_size = converted_samples * 2 * sizeof(int16_t); // 2 channels, 16-bit
                        m_audioData.append((const char*)output_buffer, buffer_size);
                    }
                    
                    av_freep(&output_buffer);
                }
            }
        }
        av_packet_unref(m_packet);
    }
    
    qDebug() << "FFmpegAudioEngine: Decoded" << m_audioData.size() << "bytes of audio data";
    qDebug() << "FFmpegAudioEngine: Duration:" << (m_duration / 1000.0) << "seconds";
    
    return AudioResult(); // Success
#else
    QString errorMsg = "FFmpeg not available";
    emit audioError(AudioError::UnsupportedFormat, errorMsg);
    return AudioResult(AudioError::UnsupportedFormat, errorMsg);
#endif
}

void FFmpegAudioEngine::setupAudioOutput()
{
    qDebug() << "FFmpegAudioEngine: Setting up audio output...";
    
    // Clean up existing audio sink
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
    }
    
    // Create new audio sink with hardware-driven callback
    m_audioSink = new QAudioSink(m_audioFormat, this);
    m_audioSink->setBufferSize(8192); // Larger buffer for stable playback
    m_audioSink->setVolume(m_volume);
    
    // Connect to hardware-driven notify signal for real-time audio processing
    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QAudio::State state) {
        qDebug() << "FFmpegAudioEngine: Audio state changed to" << state;
    });
    
    // Create custom AudioIODevice for hardware-driven callbacks
    if (m_audioDevice) {
        delete m_audioDevice;
    }
    m_audioDevice = new AudioIODevice(m_audioBuffer, this, this);
    
    qDebug() << "FFmpegAudioEngine: QAudioSink buffer size set to" << m_audioSink->bufferSize() << "bytes";
    qDebug() << "FFmpegAudioEngine: Audio format:" << m_audioFormat.sampleRate() << "Hz," << m_audioFormat.channelCount() << "channels," << m_audioFormat.sampleFormat();
    
    // Setup audio buffer
    m_audioBuffer->close();
    m_audioBuffer->setData(m_audioData);
    m_audioBuffer->open(QIODevice::ReadOnly);
    
    qDebug() << "FFmpegAudioEngine: Audio buffer setup - size:" << m_audioData.size() << "bytes";
    qDebug() << "FFmpegAudioEngine: Audio buffer open:" << m_audioBuffer->isOpen();
    qDebug() << "FFmpegAudioEngine: Audio buffer position:" << m_audioBuffer->pos();
    
    qDebug() << "FFmpegAudioEngine: Audio output setup complete";
}

void FFmpegAudioEngine::play()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "FFmpegAudioEngine: Starting playback...";
    qDebug() << "FFmpegAudioEngine: Current position:" << m_currentPosition << "ms";
    
    if (m_audioData.isEmpty()) {
        qDebug() << "FFmpegAudioEngine: No audio data loaded";
        emit audioError(AudioError::FileNotFound, "No audio file loaded");
        return;
    }
    
    if (!m_audioSink) {
        setupAudioOutput();
    }
    
    // CRITICAL: Ensure audio buffer is positioned correctly before starting playback
    if (m_audioBuffer && m_duration > 0) {
        qint64 bytePosition = (m_currentPosition * m_audioData.size()) / m_duration;
        m_audioBuffer->seek(bytePosition);
        qDebug() << "FFmpegAudioEngine: Positioned audio buffer to byte" << bytePosition << "for playback from" << (m_currentPosition / 1000.0) << "seconds";
        qDebug() << "FFmpegAudioEngine: Audio buffer position after seek:" << m_audioBuffer->pos();
    }
    
    // Start audio output with our custom device
    qDebug() << "FFmpegAudioEngine: Starting audio sink with custom device";
    qDebug() << "FFmpegAudioEngine: Audio device valid:" << (m_audioDevice != nullptr);
    qDebug() << "FFmpegAudioEngine: Audio device open:" << (m_audioDevice ? m_audioDevice->isOpen() : false);
    
    m_audioSink->start(m_audioDevice);
    
    qDebug() << "FFmpegAudioEngine: Audio sink state after start:" << m_audioSink->state();
    qDebug() << "FFmpegAudioEngine: Audio sink error:" << m_audioSink->error();
    
    if (m_audioSink->state() != QAudio::ActiveState) {
        qDebug() << "FFmpegAudioEngine: Failed to start audio sink - state:" << m_audioSink->state();
        emit audioError(AudioError::DeviceError, "Failed to start audio output");
        return;
    }
    
    // Record playback start time accounting for current position
    m_playbackStartTime = QTime::currentTime().msecsSinceStartOfDay() - m_currentPosition;
    qDebug() << "FFmpegAudioEngine: Playback start time adjusted for position:" << m_playbackStartTime;
    
    // Start position timer
    m_positionTimer->start();
    
    m_isPlaying = true;
    m_isPaused = false;
    
    emit playbackStateChanged(true);
    qDebug() << "FFmpegAudioEngine: Playback started successfully from position" << (m_currentPosition / 1000.0) << "seconds";
}

void FFmpegAudioEngine::stop()
{
    // Stop position timer FIRST to prevent further updates
    m_positionTimer->stop();
    
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "FFmpegAudioEngine: Stopping playback...";
    
    // Set flags immediately to stop all processing
    m_isPlaying = false;
    m_isPaused = false;
    
    if (m_audioSink) {
        m_audioSink->stop();
    }
    
    m_currentPosition = 0;
    
    // Reset audio buffer position
    if (m_audioBuffer) {
        m_audioBuffer->seek(0);
    }
    
    emit playbackStateChanged(false);
    emit positionChanged(0.0);
    qDebug() << "FFmpegAudioEngine: Playback stopped";
}

void FFmpegAudioEngine::pause()
{
    // Stop position timer FIRST to prevent further updates
    m_positionTimer->stop();
    
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "FFmpegAudioEngine: Pausing playback...";
    
    // Set flags immediately to stop all processing
    m_isPlaying = false;
    m_isPaused = true;
    
    if (m_audioSink) {
        m_audioSink->suspend();
    }
    
    emit playbackStateChanged(false);
    qDebug() << "FFmpegAudioEngine: Playback paused";
}

void FFmpegAudioEngine::onAudioStateChanged(QAudio::State state)
{
    qDebug() << "FFmpegAudioEngine: Audio state changed to" << state;
    
    switch (state) {
    case QAudio::ActiveState:
        qDebug() << "FFmpegAudioEngine: Audio is now active and should be playing";
        break;
    case QAudio::SuspendedState:
        qDebug() << "FFmpegAudioEngine: Audio is suspended/paused";
        break;
    case QAudio::StoppedState:
        qDebug() << "FFmpegAudioEngine: Audio has stopped";
        m_isPlaying = false;
        m_positionTimer->stop();
        emit playbackStateChanged("stopped");
        break;
    case QAudio::IdleState:
        qDebug() << "FFmpegAudioEngine: Audio is idle - no data available or underrun";
        qDebug() << "FFmpegAudioEngine: This usually means AudioIODevice isn't providing data";
        // Check if we have data available
        if (m_audioDevice) {
            qDebug() << "FFmpegAudioEngine: AudioIODevice bytes available:" << m_audioDevice->bytesAvailable();
        }
        break;
    }
}

void FFmpegAudioEngine::onPlaybackComplete()
{
    qDebug() << "FFmpegAudioEngine: Playback completed";
    stop();
}

void FFmpegAudioEngine::updatePosition()
{
    if (!m_isPlaying) {
        return;
    }
    
    // Remove mutex lock to prevent blocking in timer callback
    // Calculate position based on elapsed time
    qint64 currentTime = QTime::currentTime().msecsSinceStartOfDay();
    qint64 elapsedTime = currentTime - m_playbackStartTime;
    
    // Update position atomically
    m_currentPosition = elapsedTime;
    
    double seconds = elapsedTime / 1000.0;
    qDebug() << "FFmpegAudioEngine: Position update:" << seconds << "seconds";
    emit positionChanged(seconds);
}

// Transport control slots
void FFmpegAudioEngine::onTransportPlay()
{
    if (isPaused()) {
        play(); // Resume from pause
    } else if (!isPlaying()) {
        play(); // Start playback
    }
}

void FFmpegAudioEngine::onTransportStop()
{
    pause(); // Pause without returning to start
}

void FFmpegAudioEngine::onTransportPause()
{
    pause();
}

void FFmpegAudioEngine::onTransportStopAndReturn()
{
    stop(); // Stop and return to start
}

void FFmpegAudioEngine::onPositionChanged(double seconds)
{
    setTimelinePosition(seconds);
}

// Getters
bool FFmpegAudioEngine::isPlaying() const
{
    // Remove mutex lock from getter to prevent blocking
    return m_isPlaying;
}

bool FFmpegAudioEngine::isPaused() const
{
    // Remove mutex lock from getter to prevent blocking
    return m_isPaused;
}

double FFmpegAudioEngine::getCurrentPosition() const
{
    // Remove mutex lock from getter to prevent blocking
    return m_currentPosition / 1000.0;
}

double FFmpegAudioEngine::getDuration() const
{
    // Remove mutex lock from getter to prevent blocking
    return m_duration / 1000.0;
}

float FFmpegAudioEngine::getVolume() const
{
    // Remove mutex lock from getter to prevent blocking
    return m_volume;
}

bool FFmpegAudioEngine::isMuted() const
{
    // Remove mutex lock from getter to prevent blocking
    return m_muted;
}

// Setters
void FFmpegAudioEngine::setTimelinePosition(double seconds)
{
    // Prevent seeking during playback to avoid feedback loop
    if (m_isPlaying) {
        qDebug() << "FFmpegAudioEngine: Ignoring setTimelinePosition during playback to prevent feedback loop";
        return;
    }
    
    qDebug() << "FFmpegAudioEngine: setTimelinePosition called with" << seconds << "seconds";
    
    m_currentPosition = static_cast<qint64>(seconds * 1000.0);
    
    // Seek in audio buffer if possible
    if (m_audioBuffer && m_duration > 0) {
        qint64 bytePosition = (m_currentPosition * m_audioData.size()) / m_duration;
        m_audioBuffer->seek(bytePosition);
        m_playbackStartTime = QTime::currentTime().msecsSinceStartOfDay() - m_currentPosition;
        qDebug() << "FFmpegAudioEngine: Seeked to byte position" << bytePosition;
    }
}

void FFmpegAudioEngine::setVolume(float volume)
{
    QMutexLocker locker(&m_mutex);
    
    m_volume = qBound(0.0f, volume, 1.0f);
    
    if (m_audioSink) {
        m_audioSink->setVolume(m_volume);
    }
}

void FFmpegAudioEngine::setMuted(bool muted)
{
    QMutexLocker locker(&m_mutex);
    
    m_muted = muted;
    
    if (m_audioSink) {
        m_audioSink->setVolume(muted ? 0.0f : m_volume);
    }
}

void FFmpegAudioEngine::clearAudio()
{
    QMutexLocker locker(&m_mutex);
    
    stop();
    cleanupFFmpeg();
    m_audioData.clear();
    m_duration = 0;
    m_currentPosition = 0;
}
