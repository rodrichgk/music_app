// AudioItem.h

#ifndef AUDIOITEM_H
#define AUDIOITEM_H

#include <QGraphicsRectItem>
#include <QColor>
#include <QBrush>

class AudioItem : public QObject,public QGraphicsRectItem {
    Q_OBJECT
public:
    explicit AudioItem(int trackNumber, qreal startTime, qreal duration, const QColor& color, int trackHeight, QGraphicsItem* parent);
    ~AudioItem();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setStartTime(qreal startTime);
    qreal startTime() const;

    void setDuration(qreal duration);
    qreal duration() const;

    void setTrackNumber(int trackNumber);
    int trackNumber() const;

    void setColor(const QColor &color);
    QColor color() const;

    void updateGeometry(qreal startTime, qreal duration);

private:
    int m_trackNumber;
    QPointF m_initialPos;
    QPointF m_pressPos;
    qreal m_startTime;
    qreal m_duration;
    QColor m_color;
    int m_trackHeight; // New private member variable to store track height
    QPointF m_lastPos; // Store last mouse press position for dragging
    // Mouse event handlers
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    // Item change event handler
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

signals:
    void positionChanged(const QPointF& newPosition);
    void itemMoved(AudioItem* item);
    void currentItem(AudioItem* item);

};

#endif // AUDIOITEM_H
