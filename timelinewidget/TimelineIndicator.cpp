#include "TimelineIndicator.h"
#include <QDebug>

TimelineIndicator::TimelineIndicator(qreal height, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_height(height) {

    setZValue(1);
    setFlags(ItemIsMovable | ItemSendsGeometryChanges|ItemIsSelectable);
}

QRectF TimelineIndicator::boundingRect() const {
    return QRectF(-10, 0, 20, m_height); // Adjust the width as needed
}

void TimelineIndicator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // Draw the triangle
    // Define the triangle
    QPolygonF triangle;
    triangle << QPointF(-10, 0) << QPointF(10, 0) << QPointF(0, 20);
    QPen pen(Qt::green, 1); // Adjust color and thickness as needed
    painter->setPen(pen);

    QBrush brush(Qt::green); // Use the same color as the pen for a solid fill
    painter->setBrush(brush);
    painter->drawPolygon(triangle);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(QPointF(0, 20), QPointF(0, m_height));
}

void TimelineIndicator::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsItem::mouseMoveEvent(event);
    emit indicatorMoved(this);
}

void TimelineIndicator::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsItem::mouseReleaseEvent(event);
}

void TimelineIndicator::mousePressEvent(QGraphicsSceneMouseEvent *event){

    QGraphicsItem::mousePressEvent(event);
}

QVariant TimelineIndicator::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemPositionChange){
        if(value.toPointF().x() < 0)
            return QPointF(0,pos().y());
        return QPointF(value.toPointF().x(),pos().y());
    }
    return QGraphicsItem::itemChange(change,value);
}
