#ifndef TRACK_H
#define TRACK_H

#include <QList>
#include <QGraphicsItem>
#include "audioitem.h"
#include <QObject>

class Track : public QObject ,public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit Track(int trackHeight, qreal trackWidth);
    ~Track() override;

    void setName(const QString& name);
    QString name() const;

    int getTrackHeight();

    void updateTrackWidth(qreal trackWidth);

    void addAudioItem(AudioItem* item);
    QList<AudioItem*> audioItems() const;

    void setMute(bool mute);
    bool isMute() const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

public slots:
    void handleAudioItemPositionChange(const QPointF& newPosition);

private:
    QString m_name;
    QList<AudioItem*> m_audioItems;
    bool m_mute;
    int m_trackHeight;
    int m_trackWidth;
};

#endif // TRACK_H
