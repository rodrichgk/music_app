#include "timelinewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDebug>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFileInfo>

#if HAVE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#endif
#include <QDebug>
#include <QScrollBar>
#include <QPoint>
#include <QTime>

TimelineWidget::TimelineWidget(QWidget* parent) :
    QWidget(parent),
    m_trackHeight(AppConfig::instance().getTrackHeight()),
    m_trackPosY(0),
    scene_height(AppConfig::instance().getSceneHeight()),
    scene_width(AppConfig::instance().getSceneWidth()),
    scrollLeft(false),
    scrollRight(false),
    m_trackIdWidth(AppConfig::instance().getTrackIdWidth()),
    m_isMoving(false)
{
    // Calculate time indicator height
    QFont timeFont("Arial", 10);
    QFontMetrics fm(timeFont);
    m_timeIndicatorHeight = fm.height() + 5; // Add some padding
    
    setupUi();
    createTracksAndItems();
    setupConnections();
}

void TimelineWidget::createTracksAndItems() {
    // Create empty tracks for a clean timeline
    int tracks_to_create = (scene_height - m_trackHeight)/m_trackHeight;
    for (int i = 0; i < tracks_to_create; ++i) {
        Track* newTrack = new Track(m_trackHeight, scene_width);
        newTrack->setName(QString("Track %1").arg(i + 1));

        // COMMENTED OUT: Random audio item generation for demo purposes
        // Users should add audio items through the UI instead
        /*
        qreal startTime = QRandomGenerator::global()->bounded(0, 10) * 100; // Random start time
        qreal duration = QRandomGenerator::global()->bounded(2, 8) * 100; // Random duration
        QColor color = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 200, 200);
        AudioItem* newItem = new AudioItem(i, startTime, duration, color, m_trackHeight, newTrack);
        newItem->setPos(startTime, 0);
        newTrack->addAudioItem(newItem);
        QObject::connect(newItem,&AudioItem::currentItem,this,&TimelineWidget::setCurrentItem);
        */

        addTrack(newTrack);
    }
}

void TimelineWidget::addAudioItemToTrack(const QString& filePath, int trackIndex, const QColor& itemColor) {
    qDebug() << "=== TimelineWidget::addAudioItemToTrack START ===";
    qDebug() << "File path:" << filePath;
    qDebug() << "Track index:" << trackIndex;
    qDebug() << "Item color:" << itemColor.name();
    qDebug() << "Number of tracks available:" << m_tracks.size();
    
    // Ensure we have tracks and the requested track exists
    if (m_tracks.isEmpty() || trackIndex >= m_tracks.size() || trackIndex < 0) {
        qDebug() << "ERROR: Cannot add audio item: invalid track index" << trackIndex << "or no tracks available";
        return;
    }
    
    Track* targetTrack = m_tracks[trackIndex];
    if (!targetTrack) {
        qDebug() << "ERROR: Cannot add audio item: target track is null";
        return;
    }
    qDebug() << "Target track found and valid";
    
    // Get real audio file duration
    qreal actualDuration = getAudioFileDuration(filePath);
    
    // Create audio item at timeline start position (0,0)
    qreal startTime = 0.0; // Start at beginning of timeline
    qreal duration = actualDuration > 0 ? actualDuration : 300.0; // Use actual duration or fallback to 3 seconds
    QColor color = QColor::fromHsv(120, 180, 220); // Nice blue-green color for loaded audio
    
    qDebug() << "=== AUDIO FILE DURATION DETECTION ===";
    qDebug() << "File:" << filePath;
    qDebug() << "Raw duration from getAudioFileDuration():" << actualDuration << "seconds";
    qDebug() << "Final duration being used:" << duration << "seconds";
    qDebug() << "Duration in pixels (100px/sec):" << (duration * 100.0) << "pixels";
    qDebug() << "Duration in minutes:" << (duration / 60.0) << "minutes";
    qDebug() << "=========================================";
    
    qDebug() << "Creating AudioItem with parameters:";
    qDebug() << "  - trackIndex:" << trackIndex;
    qDebug() << "  - startTime:" << startTime;
    qDebug() << "  - duration:" << duration;
    qDebug() << "  - trackHeight:" << m_trackHeight;
    
    AudioItem* audioItem = new AudioItem(trackIndex, startTime, duration, itemColor, m_trackHeight, nullptr);
    if (!audioItem) {
        qDebug() << "ERROR: Failed to create audio item";
        return;
    }
    
    // Set the time indicator height for proper positioning calculations
    audioItem->setTimeIndicatorHeight(m_timeIndicatorHeight);
    qDebug() << "AudioItem created successfully";
    
    // Load waveform data from the audio file
    qDebug() << "Loading waveform data...";
    AudioResult waveformResult = audioItem->loadaudiowaveform(filePath);
    if (!waveformResult.isSuccess()) {
        qDebug() << "WARNING: Could not load waveform data:" << waveformResult.getErrorMessage();
        // Continue anyway - the item will still be created without waveform visualization
    } else {
        qDebug() << "Waveform data loaded successfully";
    }
    
    // === POSITIONING MEASUREMENTS LOG ===
    qDebug() << "=== AUDIO ITEM INITIAL POSITIONING ===";
    qDebug() << "Timeline measurements:";
    qDebug() << "  - m_timeIndicatorHeight:" << m_timeIndicatorHeight;
    qDebug() << "  - m_trackHeight:" << m_trackHeight;
    qDebug() << "  - scene_height:" << scene_height;
    qDebug() << "  - Number of tracks:" << m_tracks.size();
    
    qDebug() << "Audio item parameters:";
    qDebug() << "  - Target track index:" << trackIndex;
    qDebug() << "  - Start time:" << startTime;
    qDebug() << "  - Duration:" << duration;
    
    // Calculate Y position with detailed logging
    qreal yPos = m_timeIndicatorHeight + (trackIndex * m_trackHeight);
    qDebug() << "Position calculation:";
    qDebug() << "  - Formula: m_timeIndicatorHeight + (trackIndex * m_trackHeight)";
    qDebug() << "  - Calculation:" << m_timeIndicatorHeight << " + (" << trackIndex << " * " << m_trackHeight << ")";
    qDebug() << "  - Result Y position:" << yPos;
    
    // Log track boundaries for reference
    for (int i = 0; i < m_tracks.size(); ++i) {
        qreal trackStartY = m_timeIndicatorHeight + (i * m_trackHeight);
        qreal trackEndY = trackStartY + m_trackHeight;
        qDebug() << "  - Track" << i << "boundaries: Y" << trackStartY << "to" << trackEndY;
    }
    
    audioItem->setPos(startTime, yPos);
    qDebug() << "Audio item positioned at scene coordinates (" << startTime << "," << yPos << ")";
    qDebug() << "=== END INITIAL POSITIONING ===\n";
    
    // Add to track and scene
    qDebug() << "Adding audio item to track...";
    targetTrack->addAudioItem(audioItem);
    qDebug() << "Audio item added to track";
    
    qDebug() << "Adding audio item to scene...";
    if (!m_scene) {
        qDebug() << "ERROR: Scene is null!";
        return;
    }
    m_scene->addItem(audioItem);
    qDebug() << "Audio item added to scene";
    
    // Connect signals with detailed logging
    qDebug() << "Connecting signals for item:" << (void*)audioItem;
    qDebug() << "Connecting currentItem signal...";
    QObject::connect(audioItem, &AudioItem::currentItem, this, &TimelineWidget::setCurrentItem);
    qDebug() << "Connecting removeRequested signal...";
    QObject::connect(audioItem, &AudioItem::removeRequested, this, &TimelineWidget::removeAudioItem);
    qDebug() << "All signals connected for item:" << (void*)audioItem;
    
    qDebug() << "Successfully added audio item to track" << trackIndex << "from file:" << filePath;
    qDebug() << "=== TimelineWidget::addAudioItemToTrack END ===";
}

int TimelineWidget::getTrackCount() const
{
    return m_tracks.size();
}

void TimelineWidget::onTrackMuteToggled(bool muted)
{
    // Find which track header widget sent the signal
    TrackHeaderWidget* senderWidget = qobject_cast<TrackHeaderWidget*>(sender());
    if (!senderWidget) {
        qDebug() << "TimelineWidget: Could not identify sender of mute toggle signal";
        return;
    }
    
    qDebug() << "TimelineWidget: Track mute toggled to" << muted;
    
    // The track model is already updated by the TrackHeaderWidget
    // Here we could add additional logic like:
    // - Update audio engine to actually mute the track
    // - Visual feedback in the timeline
    // - Notify other components
}

void TimelineWidget::openTrackSettingsDialog(Track* track)
{
    if (!track) {
        qDebug() << "TimelineWidget: Cannot open settings for null track";
        return;
    }
    
    qDebug() << "TimelineWidget: Opening settings dialog for track" << track->getIndex();
    
    // Create and show the track settings dialog
    TrackSettingsDialog* dialog = new TrackSettingsDialog(track, this);
    
    // Connect to apply changes in real-time if needed
    // connect(dialog, &TrackSettingsDialog::settingsChanged, this, &TimelineWidget::onTrackSettingsChanged);
    
    // Show dialog modally
    int result = dialog->exec();
    
    if (result == QDialog::Accepted) {
        qDebug() << "TimelineWidget: Track settings dialog accepted";
        // Settings are already applied by the dialog
        // Could add additional processing here if needed
    } else {
        qDebug() << "TimelineWidget: Track settings dialog cancelled";
    }
    
    // Clean up
    dialog->deleteLater();
}

void TimelineWidget::setupUi() {

    initializelayout();
    configureSplitter();
    setupTrackList();
    setupGraphicsView();
    addTimeIndicators();
    addSecondLines();

    QWidget* trackListContainer = new QWidget();
    QVBoxLayout* trackListLayout = new QVBoxLayout(trackListContainer);
    trackListLayout->setContentsMargins(0, 0, 0, 0); // Remove any extra padding
    trackListLayout->setSpacing(0);
    trackListLayout->addSpacing(m_timeIndicatorHeight);
    trackListLayout->addWidget(m_trackList);
    m_splitter->addWidget(trackListContainer);
    m_splitter->addWidget(m_view);
}

void TimelineWidget::setupConnections() {
    // Here, connect signals and slots for interaction, e.g., track mute toggles.
    scrollTimer = new QTimer(this);
    connect(scrollTimer, &QTimer::timeout, this, &TimelineWidget::performScroll);
    scrollTimer->start(20);

    m_playTimer = new QTimer(this);
    connect(m_playTimer, &QTimer::timeout, this, &TimelineWidget::moveIndicator);
    m_playTimer->setInterval(16); // 60fps (16ms interval)

    // Create and setup timeline indicator after scene is ready
    if (m_scene) {
        // Ensure only one indicator exists
        if (m_indicator) {
            m_scene->removeItem(m_indicator);
            delete m_indicator;
            m_indicator = nullptr;
        }
        
        m_indicator = new TimelineIndicator(m_scene->height());
        m_indicator->setZValue(100); // Ensure it's on top
        m_scene->addItem(m_indicator);
        m_indicator->setPos(0, m_timeIndicatorHeight);
        
        QObject::connect(m_indicator,&TimelineIndicator::indicatorMoved,this,&TimelineWidget::onIndicatorMoved);
    }
    
    // Synchronize scrolling between track list and timeline
    synchronizeScrollBars();

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
    m_trackList->setFixedWidth(m_trackIdWidth);
    m_trackList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Sync with timeline scroll
    m_trackList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_trackList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_trackList->setSpacing(0);
    m_trackList->setStyleSheet("QListWidget { border: none; background: transparent; }");
}

void TimelineWidget::setupGraphicsView(){
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, scene_width, scene_height); // Placeholder values for scene dimensions
    m_view = new QGraphicsView(m_scene);
    m_view->setMinimumWidth(400);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setContentsMargins(0, 0, 0, 0);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void TimelineWidget::addTimeIndicators(){
    QFont font("Arial", 9, QFont::Medium);
    QColor textColor(200, 200, 200); // Light gray for modern look
    
    for (int i = 0; i <= m_scene->width() / 100; ++i) {
        // Convert seconds to MM:SS format
        int totalSeconds = i;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        QString timeText = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
        
        QGraphicsTextItem* timeIndicator = new QGraphicsTextItem(timeText);
        timeIndicator->setFont(font);
        timeIndicator->setDefaultTextColor(textColor);
        timeIndicator->setPos(i * 100 + 5, 2); // Small offset for better positioning
        m_scene->addItem(timeIndicator);
    }
}

void TimelineWidget::addSecondLines(){
    // Major time lines (every second)
    QPen majorPen(QColor(100, 100, 100), 1); // Dark gray
    for (int i = 0; i <= m_scene->width() / 100; ++i) {
        QGraphicsLineItem* line = new QGraphicsLineItem(i * 100, m_timeIndicatorHeight, i * 100, m_scene->height());
        line->setPen(majorPen);
        line->setZValue(-1); // Behind other items
        m_scene->addItem(line);
    }
    
    // Minor time lines (every 0.5 seconds)
    QPen minorPen(QColor(60, 60, 60), 1); // Darker gray, thinner
    for (int i = 0; i <= m_scene->width() / 50; ++i) {
        if (i % 2 != 0) { // Only draw on odd numbers (0.5, 1.5, 2.5, etc.)
            QGraphicsLineItem* line = new QGraphicsLineItem(i * 50, m_timeIndicatorHeight, i * 50, m_scene->height());
            line->setPen(minorPen);
            line->setZValue(-2); // Behind major lines
            m_scene->addItem(line);
        }
    }
}

void TimelineWidget::addTrack(Track* track) {
    // Set track index for proper identification
    int trackIndex = m_tracks.size();
    track->setIndex(trackIndex);
    
    // Create TrackHeaderWidget instead of simple QListWidgetItem
    TrackHeaderWidget* headerWidget = new TrackHeaderWidget(track, this);
    
    // Connect signals from header widget
    connect(headerWidget, &TrackHeaderWidget::muteToggled, this, &TimelineWidget::onTrackMuteToggled);
    connect(headerWidget, &TrackHeaderWidget::settingsRequested, this, &TimelineWidget::openTrackSettingsDialog);
    
    // Create QListWidgetItem to hold the custom widget
    QListWidgetItem* trackItem = new QListWidgetItem(m_trackList);
    trackItem->setSizeHint(QSize(m_trackIdWidth, m_trackHeight));
    
    // Set the custom widget as the item widget
    m_trackList->setItemWidget(trackItem, headerWidget);
    
    // Position the track properly in the scene to align with track list
    track->setPos(0, trackIndex * m_trackHeight + m_timeIndicatorHeight); // Account for time indicators height
    
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

void TimelineWidget::synchronizeScrollBars() {
    // Connect timeline vertical scrollbar to track list scrolling
    connect(m_view->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &TimelineWidget::onTimelineScrolled);
}

void TimelineWidget::onTimelineScrolled(int value) {
    // Synchronize track list scrolling with timeline scrolling
    if (m_trackList->count() > 0) {
        // Calculate which track should be visible at the top based on timeline scroll
        // Account for the time indicator height offset
        int adjustedValue = qMax(0, value - m_timeIndicatorHeight);
        int topTrackIndex = adjustedValue / m_trackHeight;
        
        // Ensure we don't scroll beyond available tracks
        topTrackIndex = qMin(topTrackIndex, m_trackList->count() - 1);
        
        if (topTrackIndex >= 0 && topTrackIndex < m_trackList->count()) {
            // Calculate the exact scroll position to align track list with timeline
            QListWidgetItem* topItem = m_trackList->item(topTrackIndex);
            if (topItem) {
                // Calculate the pixel offset within the track
                int pixelOffsetInTrack = adjustedValue % m_trackHeight;
                
                // Scroll to the appropriate track and position
                m_trackList->scrollToItem(topItem, QAbstractItemView::PositionAtTop);
                
                // Fine-tune the scroll position to match the exact pixel offset
                QScrollBar* trackScrollBar = m_trackList->verticalScrollBar();
                int currentPos = trackScrollBar->value();
                trackScrollBar->setValue(currentPos + pixelOffsetInTrack);
            }
        }
    }
    
    // Ensure timeline indicator remains visible and properly rendered after scrolling
    if (m_indicator) {
        m_indicator->update();
        m_scene->update(m_indicator->boundingRect().translated(m_indicator->scenePos()));
    }
}

void TimelineWidget::onTrackListScrolled(int value) {
    // Calculate the corresponding position in the timeline
    // The timeline should scroll proportionally with the track list
    if (m_view->verticalScrollBar()->maximum() > 0) {
        // Calculate the scroll ratio based on track list scroll position
        int maxTrackListScroll = m_trackList->verticalScrollBar()->maximum();
        int maxTimelineScroll = m_view->verticalScrollBar()->maximum();
        
        // Calculate proportional scroll position for timeline
        double scrollRatio = static_cast<double>(value) / maxTrackListScroll;
        int timelineScrollPos = static_cast<int>(scrollRatio * maxTimelineScroll);
        
        // Scroll the timeline to match track list position
        m_view->verticalScrollBar()->blockSignals(true);
        m_view->verticalScrollBar()->setValue(timelineScrollPos);
        m_view->verticalScrollBar()->blockSignals(false);
    }
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
            m_playTimer->start(); // Use the interval set in setupConnections (16ms for 60fps)
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
    if (m_indicator) {
        // Use setPos instead of moveBy to avoid accumulation issues
        QPointF currentPos = m_indicator->scenePos();
        double newX = currentPos.x() + 1.33; // Adjusted for 60fps (2.0 * 20ms / 16ms)
        
        // Block signals to prevent feedback loops
        m_indicator->blockSignals(true);
        m_indicator->setPos(newX, m_timeIndicatorHeight);
        m_indicator->blockSignals(false);
        
        // Optimize scene updates - only update the indicator's region
        QRectF updateRect = m_indicator->boundingRect().translated(m_indicator->scenePos());
        m_scene->update(updateRect);
        
        // Center view much less frequently to reduce artifacts and improve performance
        static int updateCounter = 0;
        if (++updateCounter % 30 == 0) { // Only center every 30th update (0.5 seconds)
            QRectF visibleRect = m_view->mapToScene(m_view->viewport()->rect()).boundingRect();
            if (newX < visibleRect.left() + 100 || newX > visibleRect.right() - 100) {
                m_view->centerOn(newX, m_view->mapToScene(m_view->viewport()->rect().center()).y());
            }
        }
        
        // Only emit position changes during manual interaction, not during playback
        // Check if this is a user-initiated move vs audio engine position update
        static QTime lastEmit;
        QTime currentTime = QTime::currentTime();
        if (!lastEmit.isValid() || lastEmit.msecsTo(currentTime) > 33) { // Throttle to 30fps (33ms) for UI updates
            double seconds = newX / 100.0;
            
            // Only emit if not in playback mode to prevent feedback loops
            if (!m_isPlaybackMode) {
                qDebug() << "TimelineWidget: Manual indicator position change to" << seconds << "seconds";
                emit indicatorPositionChanged(seconds);
            } else {
                qDebug() << "TimelineWidget: Suppressing position emission during playback";
            }
            lastEmit = currentTime;
        }
    }
}

void TimelineWidget::setIndicatorPosition(double seconds) {
    if (m_indicator) {
        // Convert seconds to pixels (assuming 100px = 1 second)
        double xPos = seconds * 100.0;
        
        // Block signals to prevent infinite loops
        m_indicator->blockSignals(true);
        m_indicator->setPos(xPos, m_timeIndicatorHeight);
        m_indicator->blockSignals(false);
        
        // Optimize scene updates - only update the indicator's region
        QRectF updateRect = m_indicator->boundingRect().translated(m_indicator->scenePos());
        m_scene->update(updateRect);
        
        // Use the same throttled centering logic as direct indicator movement
        static QTime lastCenterTime;
        static double lastCenteredPos = -1000;
        QTime currentTime = QTime::currentTime();
        
        // Only center if enough time has passed AND position change is significant
        if ((!lastCenterTime.isValid() || lastCenterTime.msecsTo(currentTime) > 100) && 
            qAbs(xPos - lastCenteredPos) > 300) {
            QRectF visibleRect = m_view->mapToScene(m_view->viewport()->rect()).boundingRect();
            if (xPos < visibleRect.left() + 100 || xPos > visibleRect.right() - 100) {
                m_view->centerOn(xPos, m_view->mapToScene(m_view->viewport()->rect().center()).y());
                lastCenteredPos = xPos;
                lastCenterTime = currentTime;
            }
        }
    }
}

double TimelineWidget::getIndicatorPosition() const {
    if (m_indicator) {
        // Convert pixels to seconds (assuming 100px = 1 second)
        return m_indicator->scenePos().x() / 100.0;
    }
    return 0.0;
}

void TimelineWidget::startTimelineMovement() {
    m_isMoving = true;
    if (m_playTimer) {
        m_playTimer->start(); // Use the interval set in setupConnections (16ms for 60fps)
    }
}

void TimelineWidget::stopTimelineMovement() {
    m_isMoving = false;
    if (m_playTimer) {
        m_playTimer->stop();
    }
}

void TimelineWidget::onIndicatorMoved(TimelineIndicator* indicator) {
    Q_UNUSED(indicator)
    
    // Always emit the current position to ensure transport dock stays in sync
    double seconds = m_indicator->scenePos().x() / 100.0; // Convert pixels to seconds
    emit indicatorPositionChanged(seconds);
    
    // Throttle expensive operations like focusing and scene updates
    static QTime lastUpdateTime;
    QTime currentTime = QTime::currentTime();
    
    if (!lastUpdateTime.isValid() || lastUpdateTime.msecsTo(currentTime) > 33) { // Throttle to 30fps for UI responsiveness
        // Focus on the indicator with reduced frequency
        focusOnItem(m_indicator);
        lastUpdateTime = currentTime;
        
        // Optimize scene updates - only update the indicator's region
        QRectF updateRect = m_indicator->boundingRect().translated(m_indicator->scenePos());
        m_scene->update(updateRect);
    }
}

void TimelineWidget::removeAudioItem(AudioItem* item) {
    if (!item) {
        qDebug() << "ERROR: Cannot remove null audio item";
        return;
    }
    
    qDebug() << "=== SAFE AUDIO ITEM REMOVAL ===" ;
    
    // Step 1: Immediately hide the item to prevent further interaction
    item->setVisible(false);
    item->setEnabled(false);
    
    // Step 2: Disconnect ALL signals and slots to prevent callbacks
    QObject::disconnect(item, nullptr, nullptr, nullptr);
    
    // Step 3: Clear any references to this item
    if (currentItem == item) {
        currentItem = nullptr;
    }
    
    // Step 4: Remove from tracks (simple approach)
    for (Track* track : m_tracks) {
        track->removeAudioItem(item);
    }
    
    // Step 5: Remove from scene immediately
    if (m_scene && item->scene() == m_scene) {
        m_scene->removeItem(item);
    }
    
    // Step 6: Force immediate deletion instead of deleteLater
    delete item;
    
    // Step 7: Force scene update
    if (m_scene) {
        m_scene->update();
    }
    
    qDebug() << "=== AUDIO ITEM REMOVAL COMPLETE ===";
}

void TimelineWidget::setPlaybackMode(bool isPlaying) {
    m_isPlaybackMode = isPlaying;
    qDebug() << "TimelineWidget: Playback mode set to" << isPlaying;
}

qreal TimelineWidget::getAudioFileDuration(const QString& filePath) {
    qDebug() << "=== DETECTING AUDIO FILE DURATION ===";
    qDebug() << "File path:" << filePath;
    
#if HAVE_FFMPEG
    qDebug() << "Using FFmpeg for duration detection";
    
    AVFormatContext *formatContext = avformat_alloc_context();
    if (!formatContext) {
        qDebug() << "ERROR: Could not allocate format context";
        return -1.0;
    }

    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug() << "ERROR: Could not open file for duration detection";
        avformat_free_context(formatContext);
        return -1.0;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qDebug() << "ERROR: Could not find stream information";
        avformat_close_input(&formatContext);
        return -1.0;
    }

    // Get duration in seconds
    double duration = 0.0;
    if (formatContext->duration != AV_NOPTS_VALUE) {
        duration = static_cast<double>(formatContext->duration) / AV_TIME_BASE;
        qDebug() << "FFmpeg detected duration:" << duration << "seconds";
    } else {
        qDebug() << "WARNING: Could not determine duration from FFmpeg";
        duration = -1.0;
    }

    avformat_close_input(&formatContext);
    return duration;
    
#else
    qDebug() << "FFmpeg not available, using Qt Multimedia for duration detection";
    
    // Use Qt's QMediaPlayer for duration detection
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qDebug() << "ERROR: Audio file does not exist:" << filePath;
        return -1.0;
    }
    
    // Estimate duration based on file size (rough approximation)
    qint64 fileSize = fileInfo.size();
    
    // Rough estimation: assume average bitrate of 128 kbps for MP3
    // Duration (seconds) = (file size in bytes * 8) / (bitrate in bits per second)
    double estimatedDuration = (static_cast<double>(fileSize) * 8.0) / (128.0 * 1000.0);
    
    qDebug() << "File size:" << fileSize << "bytes";
    qDebug() << "Estimated duration (128kbps assumption):" << estimatedDuration << "seconds";
    
    // Clamp to reasonable values (between 1 second and 10 minutes)
    estimatedDuration = qBound(1.0, estimatedDuration, 600.0);
    
    qDebug() << "Final estimated duration:" << estimatedDuration << "seconds";
    return estimatedDuration;
#endif
}
