#include "transportdock.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QFrame>
#include <QTimer>
#include <QStyle>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QGraphicsDropShadowEffect>

TransportDock::TransportDock(QWidget *parent)
    : QWidget(parent)
    , m_isPlaying(false)
    , m_isRecording(false)
    , m_currentPosition(0.0)
    , m_updateTimer(new QTimer(this))
{
    setupUI();
    applyModernStyling();
    
    // Connect timer for position updates
    connect(m_updateTimer, &QTimer::timeout, this, &TransportDock::updateTimer);
    m_updateTimer->setInterval(100); // Update every 100ms
}

void TransportDock::setupUI() {
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(10, 5, 10, 5);
    
    setupTransportControls();
    setupTimeDisplay();
    setupProjectControls();
    setupTrackControls();
    
    setLayout(m_mainLayout);
    setFixedHeight(60);
}

void TransportDock::setupTransportControls() {
    m_transportFrame = new QFrame();
    m_transportFrame->setFrameStyle(QFrame::StyledPanel);
    m_transportFrame->setObjectName("transportFrame");
    
    QHBoxLayout* transportLayout = new QHBoxLayout(m_transportFrame);
    transportLayout->setSpacing(5);
    transportLayout->setContentsMargins(8, 5, 8, 5);
    
    // Rewind button
    m_rewindButton = new QPushButton("⏪", this);
    m_rewindButton->setToolTip("Rewind");
    m_rewindButton->setFixedSize(35, 35);
    connect(m_rewindButton, &QPushButton::clicked, this, &TransportDock::rewind);
    
    // Play/Stop button (main transport control)
    m_playStopButton = new QPushButton();
    m_playStopButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playStopButton->setToolTip("Play");
    m_playStopButton->setFixedSize(45, 45);
    m_playStopButton->setCheckable(true);
    connect(m_playStopButton, &QPushButton::clicked, this, &TransportDock::onPlayStopClicked);
    
    // Stop and Return button
    m_stopAndReturnButton = new QPushButton("⏹", this);
    m_stopAndReturnButton->setToolTip("Stop and Return to Start");
    m_stopAndReturnButton->setFixedSize(35, 35);
    connect(m_stopAndReturnButton, &QPushButton::clicked, this, &TransportDock::stopAndReturn);
    
    // Record button
    m_recordButton = new QPushButton();
    m_recordButton->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
    m_recordButton->setToolTip("Record");
    m_recordButton->setFixedSize(35, 35);
    m_recordButton->setCheckable(true);
    connect(m_recordButton, &QPushButton::clicked, this, &TransportDock::onRecordClicked);
    
    // Fast forward button
    m_fastForwardButton = new QPushButton("⏩", this);
    m_fastForwardButton->setToolTip("Fast Forward");
    m_fastForwardButton->setFixedSize(35, 35);
    connect(m_fastForwardButton, &QPushButton::clicked, this, &TransportDock::fastForward);
    
    // Add button with plus icon
    m_addButton = new QPushButton("+", this);
    m_addButton->setToolTip("Add Items");
    m_addButton->setFixedSize(35, 35);
    m_addButton->setStyleSheet("QPushButton { font-size: 18px; font-weight: bold; }");
    connect(m_addButton, &QPushButton::clicked, this, &TransportDock::showAddMenu);
    
    transportLayout->addWidget(m_rewindButton);
    transportLayout->addWidget(m_playStopButton);
    transportLayout->addWidget(m_stopAndReturnButton);
    transportLayout->addWidget(m_recordButton);
    transportLayout->addWidget(m_fastForwardButton);
    transportLayout->addWidget(m_addButton);
    
    m_mainLayout->addWidget(m_transportFrame);
}

void TransportDock::setupTimeDisplay() {
    m_timeFrame = new QFrame();
    m_timeFrame->setFrameStyle(QFrame::StyledPanel);
    m_timeFrame->setObjectName("timeFrame");
    
    QVBoxLayout* timeLayout = new QVBoxLayout(m_timeFrame);
    timeLayout->setSpacing(2);
    timeLayout->setContentsMargins(8, 5, 8, 5);
    
    // Time display
    m_timeLabel = new QLabel("00:00.000");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setObjectName("timeLabel");
    
    // Position slider
    m_positionSlider = new QSlider(Qt::Horizontal);
    m_positionSlider->setRange(0, 10000); // Will be updated based on project length
    m_positionSlider->setValue(0);
    m_positionSlider->setToolTip("Timeline Position");
    connect(m_positionSlider, &QSlider::valueChanged, this, &TransportDock::onPositionSliderChanged);
    
    // BPM control
    QHBoxLayout* bpmLayout = new QHBoxLayout();
    m_bpmLabel = new QLabel("BPM:");
    m_bpmSpinBox = new QSpinBox();
    m_bpmSpinBox->setRange(60, 200);
    m_bpmSpinBox->setValue(120);
    m_bpmSpinBox->setToolTip("Beats Per Minute");
    connect(m_bpmSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TransportDock::onBPMChanged);
    
    bpmLayout->addWidget(m_bpmLabel);
    bpmLayout->addWidget(m_bpmSpinBox);
    
    timeLayout->addWidget(m_timeLabel);
    timeLayout->addWidget(m_positionSlider);
    timeLayout->addLayout(bpmLayout);
    
    m_mainLayout->addWidget(m_timeFrame);
}

void TransportDock::setupProjectControls() {
    m_projectFrame = new QFrame();
    m_projectFrame->setFrameStyle(QFrame::StyledPanel);
    m_projectFrame->setObjectName("projectFrame");
    
    QHBoxLayout* projectLayout = new QHBoxLayout(m_projectFrame);
    projectLayout->setSpacing(5);
    projectLayout->setContentsMargins(8, 5, 8, 5);
    
    // New project
    m_newButton = new QToolButton();
    m_newButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    m_newButton->setToolTip("New Project");
    m_newButton->setFixedSize(30, 30);
    connect(m_newButton, &QToolButton::clicked, this, &TransportDock::newProject);
    
    // Open project
    m_openButton = new QToolButton();
    m_openButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    m_openButton->setToolTip("Open Project");
    m_openButton->setFixedSize(30, 30);
    connect(m_openButton, &QToolButton::clicked, this, &TransportDock::openProject);
    
    // Save project
    m_saveButton = new QToolButton();
    m_saveButton->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_saveButton->setToolTip("Save Project");
    m_saveButton->setFixedSize(30, 30);
    connect(m_saveButton, &QToolButton::clicked, this, &TransportDock::saveProject);
    
    projectLayout->addWidget(m_newButton);
    projectLayout->addWidget(m_openButton);
    projectLayout->addWidget(m_saveButton);
    
    m_mainLayout->addWidget(m_projectFrame);
}

void TransportDock::setupTrackControls() {
    m_trackFrame = new QFrame();
    m_trackFrame->setFrameStyle(QFrame::StyledPanel);
    m_trackFrame->setObjectName("trackFrame");
    
    QHBoxLayout* trackLayout = new QHBoxLayout(m_trackFrame);
    trackLayout->setSpacing(5);
    trackLayout->setContentsMargins(8, 5, 8, 5);
    
    // Add audio track
    m_addAudioButton = new QToolButton();
    m_addAudioButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    m_addAudioButton->setToolTip("Add Audio Track");
    m_addAudioButton->setFixedSize(30, 30);
    connect(m_addAudioButton, &QToolButton::clicked, this, &TransportDock::addAudioTrack);
    
    // Add MIDI track
    m_addMidiButton = new QToolButton();
    m_addMidiButton->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    m_addMidiButton->setToolTip("Add MIDI Track");
    m_addMidiButton->setFixedSize(30, 30);
    connect(m_addMidiButton, &QToolButton::clicked, this, &TransportDock::addMidiTrack);
    
    // Add instrument track
    m_addInstrumentButton = new QToolButton();
    m_addInstrumentButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    m_addInstrumentButton->setToolTip("Add Instrument Track");
    m_addInstrumentButton->setFixedSize(30, 30);
    connect(m_addInstrumentButton, &QToolButton::clicked, this, &TransportDock::addInstrumentTrack);
    
    trackLayout->addWidget(m_addAudioButton);
    trackLayout->addWidget(m_addMidiButton);
    trackLayout->addWidget(m_addInstrumentButton);
    
    m_mainLayout->addWidget(m_trackFrame);
    
    // Add stretch to push everything to the left
    m_mainLayout->addStretch();
}

void TransportDock::applyModernStyling() {
    setStyleSheet(R"(
        TransportDock {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #3a3a3a, stop: 1 #2a2a2a);
            border-top: 1px solid #555;
        }
        
        QFrame#transportFrame, QFrame#timeFrame, QFrame#projectFrame, QFrame#trackFrame {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #4a4a4a, stop: 1 #3a3a3a);
            border: 1px solid #555;
            border-radius: 8px;
            margin: 2px;
        }
        
        QPushButton {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #5a5a5a, stop: 1 #4a4a4a);
            border: 1px solid #666;
            border-radius: 6px;
            color: white;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #6a6a6a, stop: 1 #5a5a5a);
            border: 1px solid #777;
        }
        
        QPushButton:pressed, QPushButton:checked {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #4a4a4a, stop: 1 #3a3a3a);
            border: 1px solid #888;
        }
        
        QPushButton#recordButton:checked {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #ff4444, stop: 1 #cc3333);
        }
        
        QToolButton {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #5a5a5a, stop: 1 #4a4a4a);
            border: 1px solid #666;
            border-radius: 4px;
            color: white;
        }
        
        QToolButton:hover {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #6a6a6a, stop: 1 #5a5a5a);
        }
        
        QLabel#timeLabel {
            color: #00ff00;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            font-weight: bold;
            background: #1a1a1a;
            border: 1px solid #333;
            border-radius: 3px;
            padding: 2px 6px;
        }
        
        QLabel {
            color: white;
            font-weight: bold;
        }
        
        QSlider::groove:horizontal {
            border: 1px solid #333;
            height: 6px;
            background: #222;
            border-radius: 3px;
        }
        
        QSlider::handle:horizontal {
            background: #00aa00;
            border: 1px solid #005500;
            width: 12px;
            border-radius: 6px;
            margin: -3px 0;
        }
        
        QSlider::handle:horizontal:hover {
            background: #00cc00;
        }
        
        QSpinBox {
            background: #333;
            border: 1px solid #555;
            border-radius: 3px;
            color: white;
            padding: 2px;
            min-width: 50px;
        }
        
        QSpinBox:focus {
            border: 1px solid #00aa00;
        }
    )");
    
    // Add subtle drop shadow effect
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
}

// Transport control implementations
void TransportDock::onPlayStopClicked() {
    if (m_isPlaying) {
        stop();
    } else {
        play();
    }
}

void TransportDock::play() {
    m_isPlaying = true;
    m_playStopButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    m_playStopButton->setToolTip("Pause");
    m_playStopButton->setChecked(true);
    m_updateTimer->start(50); // Update every 50ms for smooth playback
    emit playRequested();
}

void TransportDock::stop() {
    m_isPlaying = false;
    m_playStopButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playStopButton->setToolTip("Play");
    m_playStopButton->setChecked(false);
    m_updateTimer->stop();
    emit stopRequested();
}

void TransportDock::pause() {
    m_isPlaying = false;
    m_playStopButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playStopButton->setToolTip("Play");
    m_playStopButton->setChecked(false);
    m_updateTimer->stop();
    emit pauseRequested();
}

void TransportDock::onRecordClicked() {
    m_isRecording = !m_isRecording;
    m_recordButton->setChecked(m_isRecording);
    m_recordButton->setObjectName(m_isRecording ? "recordButton" : "");
    style()->polish(m_recordButton); // Refresh styling
    emit recordRequested();
}

void TransportDock::record() {
    m_isRecording = true;
    m_recordButton->setChecked(true);
    emit recordRequested();
}

void TransportDock::rewind() {
    setPosition(0.0);
}

void TransportDock::fastForward() {
    // Fast forward by 10 seconds
    setPosition(m_currentPosition + 10.0);
}

void TransportDock::stopAndReturn() {
    // Stop playback and return to start position
    m_isPlaying = false;
    m_playStopButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_playStopButton->setToolTip("Play");
    m_playStopButton->setChecked(false);
    m_updateTimer->stop();
    
    // Return to start position (0.0 seconds)
    setPosition(0.0);
    
    emit stopAndReturnRequested();
}

void TransportDock::setPosition(double seconds) {
    qDebug() << "TransportDock::setPosition called with seconds:" << seconds;
    qDebug() << "  - Current position was:" << m_currentPosition;
    
    m_currentPosition = seconds;
    updateTimeDisplay();
    qDebug() << "  - Time display updated";
    
    // Update slider without triggering signal
    m_positionSlider->blockSignals(true);
    m_positionSlider->setValue(static_cast<int>(seconds * 100)); // Convert to slider scale
    m_positionSlider->blockSignals(false);
    qDebug() << "  - Position slider updated to value:" << static_cast<int>(seconds * 100);
    
    emit positionChanged(seconds);
    qDebug() << "  - positionChanged signal emitted";
}

void TransportDock::onPositionSliderChanged(int value) {
    double seconds = value / 100.0; // Convert from slider scale
    setPosition(seconds);
}

int TransportDock::getBPM() const {
    return m_bpmSpinBox->value();
}

void TransportDock::setBPM(int bpm) {
    m_bpmSpinBox->setValue(bpm);
}

void TransportDock::onBPMChanged(int bpm) {
    emit bpmChanged(bpm);
}

void TransportDock::updateTimer() {
    if (m_isPlaying) {
        m_currentPosition += 0.05; // Increment by 50ms to match timer interval
        updateTimeDisplay();
        
        // Update slider
        m_positionSlider->blockSignals(true);
        m_positionSlider->setValue(static_cast<int>(m_currentPosition * 100));
        m_positionSlider->blockSignals(false);
        
        // Emit position change for timeline sync
        emit positionChanged(m_currentPosition);
    }
}

void TransportDock::updateTimeDisplay() {
    m_timeLabel->setText(formatTime(m_currentPosition));
}

QString TransportDock::formatTime(double seconds) const {
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    int millisecs = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);
    
    return QString("%1:%2.%3")
           .arg(minutes, 2, 10, QChar('0'))
           .arg(secs, 2, 10, QChar('0'))
           .arg(millisecs, 3, 10, QChar('0'));
}

// Project management slots
void TransportDock::newProject() {
    emit newProjectRequested();
}

void TransportDock::openProject() {
    emit openProjectRequested();
}

void TransportDock::saveProject() {
    emit saveProjectRequested();
}

// Track management slots
void TransportDock::addAudioTrack() {
    emit audioTrackRequested();
}

void TransportDock::addMidiTrack() {
    emit midiTrackRequested();
}

void TransportDock::addInstrumentTrack() {
    emit instrumentTrackRequested();
}

void TransportDock::showAddMenu() {
    QMenu* addMenu = new QMenu(this);
    
    // Audio Track option
    QAction* audioTrackAction = new QAction("Audio Track", this);
    audioTrackAction->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    connect(audioTrackAction, &QAction::triggered, this, &TransportDock::addAudioTrack);
    addMenu->addAction(audioTrackAction);
    
    // MIDI Track option
    QAction* midiTrackAction = new QAction("MIDI Track", this);
    midiTrackAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(midiTrackAction, &QAction::triggered, this, &TransportDock::addMidiTrack);
    addMenu->addAction(midiTrackAction);
    
    // Instrument Track option
    QAction* instrumentTrackAction = new QAction("Instrument Track", this);
    instrumentTrackAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    connect(instrumentTrackAction, &QAction::triggered, this, &TransportDock::addInstrumentTrack);
    addMenu->addAction(instrumentTrackAction);
    
    addMenu->addSeparator();
    
    // Audio File option
    QAction* audioFileAction = new QAction("Load Audio File...", this);
    audioFileAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(audioFileAction, &QAction::triggered, [this]() {
        // Emit a signal that MainWindow can catch to show file dialog
        emit loadAudioFileRequested();
    });
    addMenu->addAction(audioFileAction);
    
    // Show menu at button position
    QPoint menuPos = m_addButton->mapToGlobal(QPoint(0, m_addButton->height()));
    addMenu->exec(menuPos);
    
    // Clean up
    addMenu->deleteLater();
}
