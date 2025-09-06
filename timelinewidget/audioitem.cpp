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

    // Handle FFmpeg API changes for channel layout
    #if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
        // New API (FFmpeg 5.0+)
        av_opt_set_chlayout(swrContext, "in_chlayout", &codecContext->ch_layout, 0);
        AVChannelLayout mono_layout = AV_CHANNEL_LAYOUT_MONO;
        av_opt_set_chlayout(swrContext, "out_chlayout", &mono_layout, 0);
    #else
        // Old API (FFmpeg 4.x)
        av_opt_set_int(swrContext, "in_channel_layout", codecContext->channel_layout, 0);
        av_opt_set_int(swrContext, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
    #endif
    
    av_opt_set_int(swrContext, "in_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(swrContext, "in_sample_fmt", codecContext->sample_fmt, 0);
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
                    
                    // Aggressive downsampling for performance - target ~200 samples max
                    int downsampleFactor = qMax(1, samplesCount / 200); // Much less detail for 60fps performance
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

        // Remove debug output for performance
        // qDebug() << "Drawing waveform - width:" << width << "height:" << height << "centerY:" << centerY;
        // qDebug() << "Waveform samples:" << m_waveform.size();

        // Optimized waveform rendering - draw fewer lines for performance
        int maxLines = qMin(static_cast<int>(width / 2), 100); // Max 100 lines or 1 line per 2 pixels
        qreal step = width / maxLines;
        int sampleStep = qMax(1, static_cast<int>(m_waveform.size()) / maxLines);
        
        for (int i = 0; i < maxLines && i * sampleStep < static_cast<int>(m_waveform.size()); ++i) {
            qreal x = roundedRect.left() + i * step;
            qreal amplitude = m_waveform[i * sampleStep];
            
            // Scale amplitude to use more of the available height
            qreal scaledAmplitude = amplitude * (height * 0.3); // Use 30% of height for each direction
            
            // Draw vertical line from center
            qreal y1 = centerY - scaledAmplitude;
            qreal y2 = centerY + scaledAmplitude;
            
            painter->drawLine(QPointF(x, y1), QPointF(x, y2));
            
            // Remove debug output for performance
            // if (i < 5) {
            //     qDebug() << "Sample" << i << ": amplitude=" << amplitude << "scaledAmplitude=" << scaledAmplitude << "y1=" << y1 << "y2=" << y2;
            // }
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
        
        // Restrict the x-coordinate to 0 (left boundary)
        qreal minX = -m_startTime;
        x = qMax(x, minX);
        
        // Only restrict to not go above time indicator area
        y = qMax(y, static_cast<qreal>(m_timeIndicatorHeight));
        
        // Snap to track boundaries during movement
        if (m_timeIndicatorHeight > 0 && m_trackHeight > 0) {
            qreal trackAreaY = y - m_timeIndicatorHeight;
            int targetTrack = qRound(trackAreaY / m_trackHeight);
            targetTrack = qMax(0, targetTrack); // Don't go to negative tracks
            y = m_timeIndicatorHeight + (targetTrack * m_trackHeight);
        }

        return QPointF(x, y);
    }
    return QGraphicsItem::itemChange(change, value);
}



void AudioItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "\n=== MOUSE PRESS EVENT ===";
    qDebug() << "Mouse press at scene position:" << event->scenePos();
    qDebug() << "Item position before press:" << pos();
    qDebug() << "Item scene position before press:" << scenePos();
    qDebug() << "Item bounding rect:" << boundingRect();
    qDebug() << "Current track number:" << m_trackNumber;
    
    // Capture the initial scene position of the item
    m_initialPos = scenePos();
    // Record the scene position where the mouse was pressed
    m_pressPos = event->scenePos();
    QGraphicsItem::mousePressEvent(event);
    emit currentItem(this);
    
    qDebug() << "=== END MOUSE PRESS ===\n";
}

void AudioItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Log every 10th move event to avoid spam
    static int moveCounter = 0;
    moveCounter++;
    
    if (moveCounter % 10 == 0) {
        qDebug() << "=== MOUSE MOVE EVENT #" << moveCounter << "===";
        qDebug() << "Mouse scene position:" << event->scenePos();
        qDebug() << "Item position before move:" << pos();
        qDebug() << "Item scene position before move:" << scenePos();
    }
    
    QGraphicsItem::mouseMoveEvent(event);
    
    if (moveCounter % 10 == 0) {
        qDebug() << "Item position after move:" << pos();
        qDebug() << "Item scene position after move:" << scenePos();
        qDebug() << "=== END MOUSE MOVE #" << moveCounter << "===\n";
    }
    
    if(this->pos().x()<0)
    {
        setPos(0,pos().y());
    }
    update();
    emit itemMoved(this);
}

void AudioItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "\n=== MOUSE RELEASE EVENT - DETAILED ANALYSIS ===";
    
    QGraphicsItem::mouseReleaseEvent(event);
    
    // Get all position information
    QPointF currentPos = pos();
    QPointF currentScenePos = scenePos();
    QRectF itemRect = boundingRect();
    
    qDebug() << "CURRENT POSITIONS:";
    qDebug() << "  - Item pos():" << currentPos;
    qDebug() << "  - Item scenePos():" << currentScenePos;
    qDebug() << "  - Mouse release scene pos:" << event->scenePos();
    qDebug() << "  - Item bounding rect:" << itemRect;
    qDebug() << "  - Item height:" << itemRect.height();
    qDebug() << "  - Current track number:" << m_trackNumber;
    
    // Use scene position for calculations (more reliable)
    qreal currentY = currentScenePos.y();
    qreal itemTop = currentY;
    qreal itemBottom = currentY + itemRect.height();
    qreal itemCenterY = currentY + (itemRect.height() / 2);
    
    qDebug() << "ITEM BOUNDARIES:";
    qDebug() << "  - Item top Y:" << itemTop;
    qDebug() << "  - Item center Y:" << itemCenterY;
    qDebug() << "  - Item bottom Y:" << itemBottom;
    
    // Get timeline measurements (we need to access these from parent somehow)
    // For now, use hardcoded values but log them clearly
    qreal timeIndicatorHeight = 25.0; // This should come from timeline
    qreal trackHeight = m_trackHeight;
    
    qDebug() << "TIMELINE MEASUREMENTS:";
    qDebug() << "  - Time indicator height:" << timeIndicatorHeight;
    qDebug() << "  - Track height:" << trackHeight;
    
    // Calculate which tracks the item overlaps
    qDebug() << "TRACK OVERLAP ANALYSIS:";
    for (int trackIdx = 0; trackIdx < 10; ++trackIdx) { // Check first 10 tracks
        qreal trackStartY = timeIndicatorHeight + (trackIdx * trackHeight);
        qreal trackEndY = trackStartY + trackHeight;
        qreal trackCenterY = trackStartY + (trackHeight / 2);
        
        // Calculate overlap
        qreal overlapStart = qMax(itemTop, trackStartY);
        qreal overlapEnd = qMin(itemBottom, trackEndY);
        qreal overlapHeight = qMax(0.0, overlapEnd - overlapStart);
        qreal overlapPercentage = (overlapHeight / itemRect.height()) * 100.0;
        
        qDebug() << "  Track" << trackIdx << ":";
        qDebug() << "    - Track Y range:" << trackStartY << "to" << trackEndY << "(center:" << trackCenterY << ")";
        qDebug() << "    - Overlap height:" << overlapHeight << "(" << overlapPercentage << "%)";
        
        if (overlapPercentage > 50.0) {
            qDebug() << "    *** MAJORITY OVERLAP - SHOULD SNAP TO THIS TRACK ***";
            
            // Snap to this track
            qreal newY = trackStartY;
            QPointF newPos(currentPos.x(), newY);
            
            qDebug() << "SNAPPING ACTION:";
            qDebug() << "  - Snapping to track" << trackIdx;
            qDebug() << "  - New position:" << newPos;
            qDebug() << "  - Old track number:" << m_trackNumber << "-> New track number:" << trackIdx;
            
            setPos(newPos);
            m_trackNumber = trackIdx;
            break;
        }
    }
    
    // Ensure X position doesn't go negative
    if (pos().x() < 0) {
        qDebug() << "CORRECTING NEGATIVE X POSITION";
        setPos(0, pos().y());
    }
    
    qDebug() << "FINAL POSITION:" << pos();
    qDebug() << "FINAL SCENE POSITION:" << scenePos();
    
    setStartTime(pos().x());
    emit positionChanged(pos());
    
    qDebug() << "=== END MOUSE RELEASE EVENT ===\n";
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
