#include "audioitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QDebug>
#include <QPen>
#include <QPainter>
#include <QUrl>
#include <QAudioFormat>
#include <QFileInfo>

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
    loadaudiowaveform("/home/gabhy/Documents/CuteFish_apps/Music_App/Music_App/testfile.mp3");
    updateGeometry(startTime, duration);

}

AudioItem::~AudioItem() {
}
void AudioItem::loadaudiowaveform(const QString &filePath){
    m_waveform.clear();
    processAudioFile(filePath);
    update();
}

void AudioItem::processAudioFile(const QString &filePath) {
    AVFormatContext *formatContext = avformat_alloc_context();
    if (!formatContext) {
        qDebug() << "Could not allocate format context";
        return;
    }

    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug() << "Could not open file" << filePath;
        avformat_free_context(formatContext);
        return;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qDebug() << "Could not find stream information";
        avformat_close_input(&formatContext);
        return;
    }

    const AVCodec *codec = nullptr;
    int streamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (streamIndex < 0) {
        qDebug() << "Could not find audio stream";
        avformat_close_input(&formatContext);
        return;
    }

    AVStream *stream = formatContext->streams[streamIndex];
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        qDebug() << "Could not allocate codec context";
        avformat_close_input(&formatContext);
        return;
    }

    if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0) {
        qDebug() << "Could not copy codec parameters to codec context";
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        qDebug() << "Could not open codec";
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }

    // Initialize SwrContext for resampling
    SwrContext *swrContext = swr_alloc();
    if (!swrContext) {
        qDebug() << "Could not allocate resampler context";
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }

    av_opt_set_int(swrContext, "in_channel_layout", codecContext->channel_layout, 0);
    av_opt_set_int(swrContext, "in_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(swrContext, "in_sample_fmt", codecContext->sample_fmt, 0);

    av_opt_set_int(swrContext, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);  // For mono output
    av_opt_set_int(swrContext, "out_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

    if (swr_init(swrContext) < 0) {
        qDebug() << "Failed to initialize the resampling context";
        swr_free(&swrContext);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    if (!packet || !frame) {
        qDebug() << "Could not allocate packet or frame";
        swr_free(&swrContext);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        return;
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
                    for (int i = 0; i < samplesCount; ++i) {
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

    // Normalize the waveform
    qreal maxVal = *std::max_element(m_waveform.begin(), m_waveform.end());
    for (qreal &val : m_waveform) {
        val /= maxVal;
    }
}

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
        painter->setPen(Qt::black);
        painter->setBrush(Qt::NoBrush);

        qreal width = roundedRect.width();
        qreal height = roundedRect.height();
        qreal centerY = roundedRect.top() + height / 2;

        qreal step = width / m_waveform.size();
        for (size_t i = 0; i < m_waveform.size() - 1; ++i) {
            qreal x1 = roundedRect.left() + i * step;
            qreal x2 = roundedRect.left() + (i + 1) * step;
            qreal y1 = centerY - m_waveform[i] * height / 2;
            qreal y2 = centerY - m_waveform[i + 1] * height / 2;

            painter->drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
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
    int newTrackNumber = qRound(scenePos().y() / m_trackHeight) + 1;

    //qDebug() << "Y pos" << pos().y();
    qreal closestTrackY = (newTrackNumber * m_trackHeight) - (m_trackNumber * m_trackHeight);

    QPointF newPos(pos().x(), closestTrackY);

    // Set the position to the new position
    setPos(newPos);
    setStartTime(pos().x());

    // Emit the positionChanged signal with the new position
    emit positionChanged(newPos);
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
