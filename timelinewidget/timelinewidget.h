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
#include "TimelineIndicator.h"
#include "../src/appconfig.h"

class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    TimelineWidget(QWidget* parent = nullptr);
    void addTrack(Track* track);
    void createTracksAndItems();
    void addAudioItemToTrack(const QString& filePath, int trackIndex = 0);
    void performScroll();
    QTimer* scrollTimer;
    bool scrollLeft;
    bool scrollRight;
    
    // Transport dock integration
    void setIndicatorPosition(double seconds);
    double getIndicatorPosition() const;
private:
    QGraphicsItem *currentItem;
    QGraphicsScene* m_scene = nullptr;
    QGraphicsView* m_view = nullptr;
    QListWidget* m_trackList = nullptr; // For displaying track names and mute toggle
    QVBoxLayout* m_layout = nullptr;
    TimelineIndicator *m_indicator = nullptr;
    QTimer *m_playTimer = nullptr;
    bool m_isMoving;
    int m_trackHeight;
    int m_trackIdWidth;
    int m_trackPosY;
    int m_timeIndicatorHeight;
    int scene_width;
    int scene_height;
    QList<Track*> m_tracks;
    qreal m_zoomFactorX = 1.0;
    qreal m_zoomFactorY = 1.0;
    qreal m_zoomDelta = 0.1;
    qreal lastCenteredPos = 0.0;
    void setupUi();
    void setupConnections();
    void updateViewWidth();
    void decelerateAndCenterItem(QGraphicsItem* item);
    void ensureItemVisibility(AudioItem *item);
    void synchronizeScrollBars();

    void initializelayout();
    void configureSplitter();
    void setupTrackList();
    void setupGraphicsView();
    void addTimeIndicators();
    void addSecondLines();
    QSplitter* m_splitter;


protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void handleAudioItemPositionChange(const QPointF& newPosition);
    void focusOnItem(QGraphicsItem* item);
    void setCurrentItem(AudioItem* item);
    void moveIndicator();
    void onTrackListScrolled(int value);
    void onTimelineScrolled(int value);
    void onIndicatorMoved(TimelineIndicator* indicator);

signals:
    void indicatorPositionChanged(double seconds);
};

#endif // TIMELINEWIDGET_H
