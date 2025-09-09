#include "track.h"
#include <QPainter>

Track::Track(int trackHeight, qreal trackWidth) :
    QGraphicsItem(),
    m_trackHeight(trackHeight),
    m_mute(false),
    m_trackWidth(trackWidth),
    m_index(0),
    m_volume(1.0f),
    m_pan(0.0f),
    m_solo(false)
{

}

void Track::setIndex(int index)
{
    m_index = index;
}

Track::~Track() {
    // Don't delete audio items here - they should be managed by the scene/timeline
    m_audioItems.clear();
}

void Track::setName(const QString& name) {
    m_name = name;
}

int Track::getTrackHeight(){
    return m_trackHeight;
}

QString Track::name() const {
    return m_name;
}

void Track::addAudioItem(AudioItem* item) {
    m_audioItems.append(item);
    //connect(item, &AudioItem::positionChanged, this, &Track::handleAudioItemPositionChange);
}

bool Track::removeAudioItem(AudioItem* item) {
    int index = m_audioItems.indexOf(item);
    if (index != -1) {
        m_audioItems.removeAt(index);
        return true;
    }
    return false;
}

QList<AudioItem*> Track::audioItems() const {
    return m_audioItems;
}

void Track::setMute(bool mute) {
    m_mute = mute;
}

bool Track::isMute() const {
    return m_mute;
}

QRectF Track::boundingRect() const {
    // Adjust this as needed to ensure the entire track is within the scene's visible area
    return QRectF(0, 0, m_trackWidth, m_trackHeight);
}

void Track::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPen pen(Qt::black, 1); // Set the color and width of the pen as desired
    painter->setPen(pen);

    QRectF rect = boundingRect(); // Get the bounding rectangle of the track

    // Draw the top line
    //painter->drawLine(rect.topLeft(), rect.topRight());

    // Draw the bottom line
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());

}

void Track::handleAudioItemPositionChange(const QPointF& newPosition) {
    // Handle changes in the audio item's position
    // For example, update the display if necessary
}

void Track::updateTrackWidth(qreal trackWidth){
    prepareGeometryChange();
    m_trackWidth = trackWidth;
    update();
}

