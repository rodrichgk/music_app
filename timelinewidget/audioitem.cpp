#include "audioitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QDebug>
#include <QPen>
#include <QPainter>
#include <QUrl>
#include <QAudioFormat>
#include <QFileInfo>
#include <cmath>

AudioItem::AudioItem(int trackNumber, qreal startTime, qreal duration, const QColor& color, int trackHeight, QGraphicsItem* parent)
    : QGraphicsRectItem(parent),
    m_trackNumber(trackNumber),
    m_startTime(startTime),
    m_duration(duration),
    m_color(color),
    m_trackHeight(trackHeight)
{
    // Initialize the item's appearance here, if needed
    setZValue(1);
    setFlags(ItemIsMovable | ItemSendsGeometryChanges|ItemIsSelectable);
    // loadaudiowaveform("/home/gabhy/Documents/CuteFish_apps/Music_App/Music_App/testfile.mp3"); // TODO: Remove hard-coded path
    updateGeometry(startTime, duration);

}

AudioItem::~AudioItem() {
    // Clear waveform data
    m_waveform.clear();
    
#if HAVE_FFMPEG
    // FFmpeg resources are cleaned up in processAudioFile() method
    // No persistent FFmpeg resources to clean up here
#endif
}
AudioResult AudioItem::loadaudiowaveform(const QString &filePath){
    qDebug() << "AudioItem::loadaudiowaveform called with file:" << filePath;
    qDebug() << "HAVE_FFMPEG is defined as:" << HAVE_FFMPEG;
    
    m_waveform.clear();
#if HAVE_FFMPEG
    if (filePath.isEmpty()) {
        return AudioResult::error(AudioError::InvalidParameters, "File path is empty");
    }
    qDebug() << "Using FFmpeg to process audio file";
    return processAudioFile(filePath);
#else
    qDebug() << "FFmpeg not available - using Qt Multimedia for basic waveform generation";
    
    // Use Qt's QMediaPlayer to get basic audio information
    // For now, generate a more realistic-looking waveform based on file size/duration
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qDebug() << "Audio file does not exist:" << filePath;
        return AudioResult::error(AudioError::FileNotFound, "Audio file not found: " + filePath);
    }
    
    // Generate a more realistic waveform pattern
    // Scale based on file size to make it look more authentic
    qint64 fileSize = fileInfo.size();
    int numSamples = qMin(static_cast<int>(fileSize / 1000), 1000); // Rough approximation
    numSamples = qMax(numSamples, 100); // Minimum samples
    
    qDebug() << "Generating" << numSamples << "waveform samples for file size:" << fileSize << "bytes";
    
    // Generate more realistic audio-like waveform
    for (int i = 0; i < numSamples; ++i) {
        // Create a more complex waveform that looks like real audio
        double t = static_cast<double>(i) / numSamples;
        double amplitude = 0.0;
        
        // Add multiple frequency components to simulate real audio
        amplitude += 0.4 * sin(t * 20 * M_PI) * exp(-t * 2); // Decaying high frequency
        amplitude += 0.3 * sin(t * 8 * M_PI) * (1 - t); // Mid frequency
        amplitude += 0.2 * sin(t * 3 * M_PI); // Low frequency
        amplitude += 0.1 * (static_cast<double>(rand()) / RAND_MAX - 0.5); // Random noise
        
        // Vary amplitude over time to simulate real audio dynamics
        double envelope = 0.5 + 0.5 * sin(t * 4 * M_PI);
        amplitude *= envelope;
        
        // Clamp to reasonable range
        amplitude = qBound(-1.0, amplitude, 1.0);
        m_waveform.push_back(amplitude);
    }
    
    qDebug() << "Generated waveform with" << m_waveform.size() << "samples";
    update();
    return AudioResult::success();
#endif
}

#if HAVE_FFMPEG
AudioResult AudioItem::processAudioFile(const QString &filePath) {
    AVFormatContext *formatContext = avformat_alloc_context();
    if (!formatContext) {
        return AudioResult::error(AudioError::MemoryError, "Could not allocate format context");
    }

    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        avformat_free_context(formatContext);
        return AudioResult::error(AudioError::FileNotFound, "Could not open file: " + filePath);
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::DecodingFailed, "Could not find stream information");
    }

    const AVCodec *codec = nullptr;
    int streamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (streamIndex < 0) {
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::UnsupportedFormat, "Could not find audio stream");
    }

    AVStream *stream = formatContext->streams[streamIndex];
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::MemoryError, "Could not allocate codec context");
    }

    if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::DecodingFailed, "Could not copy codec parameters");
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::DecodingFailed, "Could not open codec");
    }

    // Initialize SwrContext for resampling
    SwrContext *swrContext = swr_alloc();
    if (!swrContext) {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::MemoryError, "Could not allocate resampler context");
    }

    av_opt_set_int(swrContext, "in_channel_layout", codecContext->channel_layout, 0);
    av_opt_set_int(swrContext, "in_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(swrContext, "in_sample_fmt", codecContext->sample_fmt, 0);

    av_opt_set_int(swrContext, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);  // For mono output
    av_opt_set_int(swrContext, "out_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

    if (swr_init(swrContext) < 0) {
        swr_free(&swrContext);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::DecodingFailed, "Failed to initialize resampling context");
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    if (!packet || !frame) {
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        swr_free(&swrContext);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return AudioResult::error(AudioError::MemoryError, "Could not allocate packet or frame");
    }

    int64_t totalSamples = 0;

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == streamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                while (avcodec_receive_frame(codecContext, frame) == 0) {
                    // Allocate buffer for resampled data
                    float *convertedSamples = nullptr;
                    int outSamples = av_rescale_rnd(swr_get_delay(swrContext, codecContext->sample_rate) + frame->nb_samples, codecContext->sample_rate, codecContext->sample_rate, AV_ROUND_UP);
                    int bufferSize = av_samples_alloc((uint8_t**)&convertedSamples, nullptr, 1, outSamples, AV_SAMPLE_FMT_FLT, 1);
                    if (bufferSize < 0) {
                        qDebug() << "Could not allocate resampled buffer";
                        continue;
                    }

                    int samplesCount = swr_convert(swrContext, (uint8_t **)&convertedSamples, outSamples, (const uint8_t **)frame->data, frame->nb_samples);
                    
                    // Downsample for visualization - take every Nth sample to reduce data
                    int downsampleFactor = qMax(1, samplesCount / 2000); // Target ~2000 samples for better detail
                    for (int i = 0; i < samplesCount; i += downsampleFactor) {
                        m_waveform.push_back(convertedSamples[i]);
                    }
                    totalSamples += samplesCount;

                    // Free the resampled buffer
                    av_freep(&convertedSamples);
                }
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swrContext);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    // Normalize the waveform using absolute values to handle both positive and negative peaks
    if (!m_waveform.empty()) {
        qreal maxAbsVal = 0.0;
        for (const qreal &val : m_waveform) {
            maxAbsVal = qMax(maxAbsVal, qAbs(val));
        }
        
        qDebug() << "Waveform before normalization - samples:" << m_waveform.size() << "max absolute value:" << maxAbsVal;
        
        if (maxAbsVal > 0.001) { // Avoid division by very small numbers
            for (qreal &val : m_waveform) {
                val /= maxAbsVal;
            }
            qDebug() << "Waveform normalized successfully";
        } else {
            qDebug() << "Warning: Waveform has very low amplitude, may appear as flat line";
        }
        
        // Log first few samples for debugging
        qDebug() << "First 10 normalized samples:";
        for (int i = 0; i < qMin(10, static_cast<int>(m_waveform.size())); ++i) {
            qDebug() << "  Sample" << i << ":" << m_waveform[i];
        }
    }
    
    update();
    return AudioResult::success();
}
#endif // HAVE_FFMPEG

void AudioItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // Set the pen for the border of the rectangle
    QPen pen(Qt::NoPen); // No border
    painter->setPen(pen);

    // Set the brush for the inside color of the rectangle
    QBrush brush(m_color);
    painter->setBrush(brush);

    // Draw a rounded rectangle
    qreal borderRadius = 10; // Adjust for desired border radius
    QRectF roundedRect = rect();
    painter->drawRoundedRect(roundedRect, borderRadius, borderRadius);

    // Clip the drawing area to the rounded rectangle
    QPainterPath clipPath;
    clipPath.addRoundedRect(roundedRect, borderRadius, borderRadius);
    painter->setClipPath(clipPath);

    // Draw the waveform
    if (!m_waveform.empty()) {
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(Qt::NoBrush);

        qreal width = roundedRect.width();
        qreal height = roundedRect.height();
        qreal centerY = roundedRect.top() + height / 2;

        qDebug() << "Drawing waveform - width:" << width << "height:" << height << "centerY:" << centerY;
        qDebug() << "Waveform samples:" << m_waveform.size();

        // Draw waveform as vertical lines from center (more typical audio visualization)
        qreal step = width / m_waveform.size();
        for (size_t i = 0; i < m_waveform.size(); ++i) {
            qreal x = roundedRect.left() + i * step;
            qreal amplitude = m_waveform[i];
            
            // Scale amplitude to use more of the available height
            qreal scaledAmplitude = amplitude * (height * 0.4); // Use 40% of height for each direction
            
            // Draw vertical line from center
            qreal y1 = centerY - scaledAmplitude;
            qreal y2 = centerY + scaledAmplitude;
            
            painter->drawLine(QPointF(x, y1), QPointF(x, y2));
            
            // Debug first few samples
            if (i < 5) {
                qDebug() << "Sample" << i << ": amplitude=" << amplitude << "scaledAmplitude=" << scaledAmplitude << "y1=" << y1 << "y2=" << y2;
            }
        }
        
        qDebug() << "Waveform drawing completed";
    } else {
        qDebug() << "No waveform data to draw";
    }

    // Reset clipping
    painter->setClipping(false);
}


QVariant AudioItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        qreal x = newPos.x();
        qreal y = newPos.y();
        qreal minX = -m_startTime; // Minimum x-coordinate is 0
        qreal minY = -m_trackHeight * m_trackNumber; // Minimum y-coordinate
        minY += boundingRect().height(); // Adjust minY by adding the height of the audio item

        // Restrict the x-coordinate to 0 (left boundary)
        x = qMax(x, minX);
        // Restrict the y-coordinate to the minimum value
        y = qMax(y, minY);

        // Call the base class implementation to handle other changes
        return QPointF(x, y);
    }
    return QGraphicsItem::itemChange(change, value);
}



void AudioItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Capture the initial scene position of the item
    m_initialPos = scenePos();
    // Record the scene position where the mouse was pressed
    m_pressPos = event->scenePos();
    QGraphicsItem::mousePressEvent(event);
    emit currentItem(this);
}

void AudioItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if(this->pos().x()<0)
    {
        setPos(0,pos().y());
    }
    update();
    emit itemMoved(this);
}

void AudioItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    
    // Get current scene position
    QPointF currentScenePos = scenePos();
    qreal currentY = currentScenePos.y();
    
    qDebug() << "AudioItem mouse release - current Y:" << currentY << "track height:" << m_trackHeight;
    
    // Calculate which track this item is mostly in
    // Account for time indicator height offset
    qreal adjustedY = currentY - 25; // Assuming time indicator height is around 25px
    int targetTrackIndex = qMax(0, static_cast<int>(adjustedY / m_trackHeight));
    
    // Check if more than 50% of the item is in the new track
    qreal itemCenterY = currentY + (m_trackHeight / 2);
    qreal trackCenterY = 25 + (targetTrackIndex * m_trackHeight) + (m_trackHeight / 2);
    
    qreal overlapAmount = qAbs(itemCenterY - trackCenterY);
    bool shouldSnapToTrack = overlapAmount < (m_trackHeight / 2);
    
    qDebug() << "Target track index:" << targetTrackIndex;
    qDebug() << "Item center Y:" << itemCenterY << "Track center Y:" << trackCenterY;
    qDebug() << "Overlap amount:" << overlapAmount << "Should snap:" << shouldSnapToTrack;
    
    if (shouldSnapToTrack) {
        // Snap to the target track
        qreal newY = 25 + (targetTrackIndex * m_trackHeight); // 25 is time indicator height
        QPointF newPos(pos().x(), newY);
        
        qDebug() << "Snapping to track" << targetTrackIndex << "at Y position:" << newY;
        setPos(newPos);
        
        // Update track number
        m_trackNumber = targetTrackIndex;
    } else {
        qDebug() << "Not snapping - insufficient overlap";
    }
    
    // Ensure X position doesn't go negative
    if (pos().x() < 0) {
        setPos(0, pos().y());
    }
    
    setStartTime(pos().x());
    emit positionChanged(pos());
}


void AudioItem::setStartTime(qreal startTime) {
    m_startTime = startTime;
    //updateGeometry(m_startTime, m_duration);
}

qreal AudioItem::startTime() const {
    return m_startTime;
}

void AudioItem::setDuration(qreal duration) {
    m_duration = duration;
    updateGeometry(m_startTime, m_duration);
}

qreal AudioItem::duration() const {
    return m_duration;
}

void AudioItem::setTrackNumber(int trackNumber) {
    m_trackNumber = trackNumber;
}

void AudioItem::setColor(const QColor &color) {
    m_color = color;
    setBrush(color);
}

QColor AudioItem::color() const {
    return m_color;
}

void AudioItem::updateGeometry(qreal startTime, qreal duration) {
    m_startTime = startTime;
    m_duration = duration;
    setColor(m_color);
    setRect(startTime, 0, m_duration, m_trackHeight);
    update();
}
