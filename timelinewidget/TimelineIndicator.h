#ifndef TIMELINEINDICATOR_H
#define TIMELINEINDICATOR_H

#include <QGraphicsItem>
#include <QPainter>

class TimelineIndicator : public QGraphicsItem {
public:
    explicit TimelineIndicator(qreal height, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    qreal m_height;
};

#endif // TIMELINEINDICATOR_H
