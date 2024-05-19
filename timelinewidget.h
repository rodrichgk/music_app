// TimelineWidget.h

#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QListWidget>
#include "audioitem.h"
#include "track.h"
#include <QTimer>
#include <QSplitter>

class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    TimelineWidget(QWidget* parent = nullptr);
    void addTrack(Track* track);
    void createTracksAndItems();
    void performScroll();
    QTimer* scrollTimer;
    bool scrollLeft;
    bool scrollRight;
private:
    QGraphicsItem *currentItem;
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    QListWidget* m_trackList; // For displaying track names and mute toggle
    QVBoxLayout* m_layout;
    int m_trackHeight;
    int m_trackIdWidth;
    int m_trackPosY;
    int scene_width;
    int scene_height;
    QList<Track*> m_tracks;
    qreal m_zoomFactorX = 1.0;
    qreal m_zoomFactorY = 1.0;
    qreal m_zoomDelta = 0.1;
    void setupUi();
    void setupConnections();
    void updateViewWidth();
    void decelerateAndCenterItem(QGraphicsItem* item);
    void ensureItemVisibility(AudioItem *item);

    void initializelayout();
    void configureSplitter();
    void setupTrackList();
    void setupGraphicsView();
    void addTimeIndicators();
    void addSecondLines();
    QSplitter* m_splitter;


protected:
    void wheelEvent(QWheelEvent *event) override;

public slots:
    void handleAudioItemPositionChange(const QPointF& newPosition);
    void focusOnItem(AudioItem* item);
    void setCurrentItem(AudioItem* item);
};

#endif // TIMELINEWIDGET_H
