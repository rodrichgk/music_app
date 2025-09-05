#ifndef TIMELINEINDICATOR_H
#define TIMELINEINDICATOR_H

#include <QGraphicsItem>
#include <QPainter>

class TimelineIndicator :public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
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

signals:
    void indicatorMoved(TimelineIndicator* indicator);
};

#endif // TIMELINEINDICATOR_H
