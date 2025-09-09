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
    bool removeAudioItem(AudioItem* item);
    QList<AudioItem*> audioItems() const;

    void setMute(bool mute);
    bool isMute() const;
    
    // New mixer properties
    int getIndex() const { return m_index; }
    void setIndex(int index);
    void setMuted(bool muted) { m_mute = muted; }
    bool isMuted() const { return m_mute; }
    void setVolume(float volume) { m_volume = volume; }
    float getVolume() const { return m_volume; }
    void setPan(float pan) { m_pan = pan; }
    float getPan() const { return m_pan; }
    void setSoloed(bool soloed) { m_solo = soloed; }
    bool isSoloed() const { return m_solo; }

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
    
    // New mixer properties
    int m_index;
    float m_volume;
    float m_pan;
    bool m_solo;
};

#endif // TRACK_H
