#include "timelinewidget.h"
#include <QGraphicsRectItem>
#include <QSplitter>
#include <QListWidgetItem>
#include <QRandomGenerator>
#include <vector>
#include <QWheelEvent>
#include <QDebug>
#include <QScrollBar>
#include <QPoint>

TimelineWidget::TimelineWidget(QWidget* parent) :
    QWidget(parent),
    m_trackHeight(50),
    m_trackPosY(0),
    scene_height(1020),
    scene_width(5000),
    scrollLeft(false),
    scrollRight(false),
    m_trackIdWidth(200),
    m_isMoving(false)
{
    setupUi();
    createTracksAndItems();
    setupConnections();
}
void TimelineWidget::createTracksAndItems() {
    // Create four tracks
    int tracks_to_create = (scene_height - m_trackHeight)/m_trackHeight;
    for (int i = 0; i < tracks_to_create; ++i) {
        QString trackName = "Track " + QString::number(i + 1);
        Track* newTrack = new Track(m_trackHeight, scene_width); // Assuming parentage is managed elsewhere
        newTrack->setName(trackName);

        // Add the track to the scene at the appropriate position
        newTrack->setPos(0, m_trackPosY + 20);

        // Increment y-coordinate for the next track
        m_trackPosY += m_trackHeight; // Adjust vertical spacing as needed

        // Add one audio item to each track with a duration of 10 seconds and a random color
        qint64 duration = 100; // Duration set to 1 seconds (1000 milliseconds)
        QColor randomColor = QColor::fromRgb(QRandomGenerator::global()->generate());
        AudioItem* newItem = new AudioItem(i + 1, 0, duration, randomColor, m_trackHeight, newTrack);
        //newItem->loadaudiowaveform("/home/gabhy/Documents/CuteFish_apps/Music_App/Music_App/testfile.mp3");
        newTrack->addAudioItem(newItem);
        QObject::connect(newItem, &AudioItem::positionChanged, this, &TimelineWidget::handleAudioItemPositionChange);
        QObject::connect(newItem,&AudioItem::itemMoved,this,&TimelineWidget::focusOnItem);
        QObject::connect(newItem,&AudioItem::currentItem,this,&TimelineWidget::setCurrentItem);
        addTrack(newTrack);
    }
    m_indicator = new TimelineIndicator(m_scene->height());

    m_scene->addItem(m_indicator);

    m_indicator->setPos(0,20);
}

void TimelineWidget::setupUi() {

    initializelayout();
    configureSplitter();
    setupTrackList();
    setupGraphicsView();
    addTimeIndicators();
    addSecondLines();

    m_splitter->addWidget(m_trackList);
    m_splitter->addWidget(m_view);
}

void TimelineWidget::setupConnections() {
    // Here, connect signals and slots for interaction, e.g., track mute toggles.
    scrollTimer = new QTimer(this);
    connect(scrollTimer, &QTimer::timeout, this, &TimelineWidget::performScroll);
    scrollTimer->start(20);

    m_playTimer = new QTimer(this);
    connect(m_playTimer, &QTimer::timeout, this, &TimelineWidget::moveIndicator);

    QObject::connect(m_indicator,&TimelineIndicator::indicatorMoved,this,&TimelineWidget::focusOnItem);

}

void TimelineWidget::initializelayout(){
    m_layout = new QVBoxLayout(this);
    setLayout(m_layout);
}

void TimelineWidget::configureSplitter(){
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_layout->addWidget(m_splitter);
}

void TimelineWidget::setupTrackList(){
    m_trackList = new QListWidget();
    m_trackList->setFixedWidth(200);
}

void TimelineWidget::setupGraphicsView(){
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, scene_width, scene_height); // Placeholder values for scene dimensions
    m_view = new QGraphicsView(m_scene);
    m_view->setMinimumWidth(400);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setContentsMargins(0, 0, 0, 0);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void TimelineWidget::addTimeIndicators(){
    QFont font("Arial", 10);
    for (int i = 0; i <= m_scene->width() / 10; ++i) {
        QGraphicsTextItem* secondsIndicator = new QGraphicsTextItem(QString::number(i));
        secondsIndicator->setFont(font);
        secondsIndicator->setPos(i * 100, 0);
        m_scene->addItem(secondsIndicator);
    }
}

void TimelineWidget::addSecondLines(){
    QPen pen(Qt::black);
    for (int i = 0; i <= m_scene->width() / 10; ++i) {
        QGraphicsLineItem* line = new QGraphicsLineItem(i * 100, 0, i * 100, m_scene->height() - m_trackHeight);
        line->setPen(pen);
        m_scene->addItem(line);
    }
}

void TimelineWidget::addTrack(Track* track) {
    // Add track info to QListWidget
    QListWidgetItem* trackItem = new QListWidgetItem(track->name(), m_trackList);
    trackItem->setSizeHint(QSize(m_trackIdWidth, track->getTrackHeight()));
    trackItem->setFlags(trackItem->flags() | Qt::ItemIsUserCheckable); // Add checkbox
    trackItem->setCheckState(Qt::Unchecked); // Default to not muted
    // Add the track to the scene
    m_scene->addItem(track);
    m_tracks.append(track);
}

void TimelineWidget::handleAudioItemPositionChange(const QPointF& newPosition) {
    updateViewWidth();
    m_scene->update();
}


void TimelineWidget::updateViewWidth() {
    int maxWidth = 0;

    // Calculate the maximum width needed for all tracks
    for (Track* track : m_tracks) {
        int trackWidth = 0;
        for (AudioItem* item : track->audioItems()) {
            int itemEndPosition = item->startTime() + item->duration(); // Assuming both are in pixels
            trackWidth = std::max(trackWidth, itemEndPosition);
        }
        maxWidth = std::max(maxWidth, trackWidth);
    }
    maxWidth += 200; // Adding some margin

    // Adjust the scene's rectangle to accommodate the new width
    QRectF currentSceneRect = m_scene->sceneRect();
    if (maxWidth > currentSceneRect.width()) {
        m_scene->setSceneRect(0, 0, maxWidth, currentSceneRect.height());

        for(Track* track: m_tracks){
            track->updateTrackWidth(maxWidth);
        }
    }

    scrollLeft = false;
    scrollRight = false;
}

void TimelineWidget::decelerateAndCenterItem(QGraphicsItem* item) {
    if (!item) return; // Ensure the item is valid

    // Target position to center the item
    qreal centerX = item->sceneBoundingRect().center().x();
    qreal viewportCenterX = m_view->viewport()->width() / 2.0;
    qreal targetScrollBarPos = centerX - viewportCenterX + m_view->horizontalScrollBar()->minimum();

    // Current scroll bar position
    int currentScrollBarPos = m_view->horizontalScrollBar()->value();

    // Calculate the difference between current and target positions
    int delta = targetScrollBarPos - currentScrollBarPos;

    // If the delta is small enough, just jump to the target and stop the timer
    if (abs(delta) <= 5) {
        m_view->horizontalScrollBar()->setValue(targetScrollBarPos);
        return; // Stop the deceleration process here
    }

    // Calculate the next step size for a smooth deceleration
    int stepSize = delta / 10; // Adjust the divisor for smoother or quicker deceleration
    if (stepSize == 0) {
        stepSize = (delta > 0) ? 1 : -1; // Ensure we always move at least a little bit
    }

    // Update the scroll bar position to move towards the center
    m_view->horizontalScrollBar()->setValue(currentScrollBarPos + stepSize);

    // Continue the deceleration process after a short delay
    QTimer::singleShot(15, this, [this, item]() { decelerateAndCenterItem(item); });
}


void TimelineWidget::focusOnItem(QGraphicsItem *item)
{
    if (item && m_view) {
        qreal itemMiddleX = item->pos().x() + (item->boundingRect().width() / 2);

        // Step 2: Convert the middle point to viewport coordinates.
        QPoint itemMiddlePointInViewport = m_view->mapFromScene(QPointF(itemMiddleX, item->pos().y()));

        // Determine margins as a fixed value or a percentage of the viewport's width
        int leftMargin = 0; // e.g., 50 pixels from the left edge of the viewport
        int rightMargin = m_view->viewport()->width() ; // e.g., 50 pixels from the right edge of the viewport

        // Step 3: Check if the middle point is within the margins
        scrollLeft = itemMiddlePointInViewport.x() <= leftMargin;
        scrollRight = itemMiddlePointInViewport.x() >= rightMargin;
    }
}

void TimelineWidget::setCurrentItem(AudioItem* item){
    currentItem = item;
}

void TimelineWidget::wheelEvent(QWheelEvent *event) {
    QPointF mousePos = event->position(); // Get mouse position relative to the widget
    QPointF sceneMousePos = m_view->mapToScene(mousePos.toPoint()); // Convert mouse position to scene coordinates
    qreal zoomFactor = 1.0 + event->angleDelta().y() / 1200.0; // Calculate zoom factor

    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // Set transformation anchor to mouse position

    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom on X-axis when Control key is pressed
        qreal scaleFactor = m_view->transform().m11() * zoomFactor;
        m_view->scale(zoomFactor, 1.0);
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // Zoom on Y-axis when Shift key is pressed
        qreal scaleFactor = m_view->transform().m22() * zoomFactor;
        m_view->scale(1.0, zoomFactor);
    } else {
        // Let the default behavior handle the event
        QWidget::wheelEvent(event);
        return;
    }

    // Center the view on the mouse position
    m_view->centerOn(sceneMousePos);
}


void TimelineWidget::ensureItemVisibility(AudioItem *item) {
    if (!item || !m_view) return;

    // Item's scene coordinates
    qreal itemLeft = item->scenePos().x();
    qreal itemRight = item->scenePos().x() + item->boundingRect().width();

    // View's visible scene rectangle
    QRectF visibleRect = m_view->mapToScene(m_view->viewport()->rect()).boundingRect();

    // Horizontal scrolling
    if (itemLeft < visibleRect.left()) {
        // Item is to the left of the visible area; scroll left
        m_view->horizontalScrollBar()->setValue(m_view->horizontalScrollBar()->value() + (itemLeft - visibleRect.left() - 10)); // Adjust for margin
    } else if (itemRight > visibleRect.right()) {
        // Item is to the right of the visible area; scroll right
        m_view->horizontalScrollBar()->setValue(m_view->horizontalScrollBar()->value() + (itemRight - visibleRect.right() + 10)); // Adjust for margin
    }
}

void TimelineWidget::performScroll(){
    static int stepSize = 1; // Start with a small step size
    static int maxStepSize = 10; // Maximum step size for acceleration
    static int acceleration = 1; // How quickly to accelerate

    if (scrollLeft || scrollRight) {
        // Accelerate scrolling up to a maximum step size
        if (stepSize < maxStepSize) {
            stepSize += acceleration;
        }
        int newValue = m_view->horizontalScrollBar()->value() + (scrollRight ? stepSize : -stepSize);
        m_view->horizontalScrollBar()->setValue(newValue);
    } else {
        // Reset step size when scrolling stops
        stepSize = 1;
    }
}

void TimelineWidget::keyPressEvent(QKeyEvent *event){
    if(event->key() == Qt::Key_Space)
    {
        m_isMoving = !m_isMoving;

        if(m_isMoving){
            m_playTimer->start(20);
        }
        else
        {
            m_playTimer->stop();
        }
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

void TimelineWidget::moveIndicator(){
    m_indicator->moveBy(2,0);
    QPointF indicatorPos = m_indicator->scenePos();
    m_view->centerOn(indicatorPos.x(), m_view->mapToScene(m_view->viewport()->rect().center()).y());
}
