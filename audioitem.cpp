#include "audioitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QDebug>
#include <QPen>
#include <QPainter>

AudioItem::AudioItem(int trackNumber, qreal startTime, qreal duration, const QColor& color, int trackHeight, QGraphicsItem* parent)
    : QGraphicsRectItem(parent), m_trackNumber(trackNumber), m_startTime(startTime), m_duration(duration), m_color(color), m_trackHeight(trackHeight)
{
    // Initialize the item's appearance here, if needed
    setZValue(1);
    setFlags(ItemIsMovable | ItemSendsGeometryChanges|ItemIsSelectable);
    updateGeometry(startTime, duration);
}

AudioItem::~AudioItem() {}

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
    painter->drawRoundedRect(rect(), borderRadius, borderRadius);
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
