#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../timelinewidget/timelinewidget.h"
#include "transportdock.h"
#include "audioengine.h"
#include <QBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_transportDock(nullptr)
    , m_audioEngine(nullptr)
{
    ui->setupUi(this);

    // Create the audio engine first
    m_audioEngine = new AudioEngine(this);
    
    // Create the transport dock
    m_transportDock = new TransportDock(this);
    
    // Create timeline widget
    m_timelineWidget = new TimelineWidget(this);

    // Create main layout
    QVBoxLayout *layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Add transport dock at the top
    layout->addWidget(m_transportDock);
    
    // Add timeline widget (main content)
    layout->addWidget(m_timelineWidget, 1); // Give timeline widget stretch factor
    
    ui->centralwidget->setLayout(layout);
    
    // Connect transport dock signals
    connect(m_transportDock, &TransportDock::playRequested, this, &MainWindow::onPlayRequested);
    connect(m_transportDock, &TransportDock::stopRequested, this, &MainWindow::onStopRequested);
    connect(m_transportDock, &TransportDock::recordRequested, this, &MainWindow::onRecordRequested);
    connect(m_transportDock, &TransportDock::stopAndReturnRequested, this, &MainWindow::onStopAndReturnRequested);
    connect(m_transportDock, &TransportDock::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_transportDock, &TransportDock::newProjectRequested, this, &MainWindow::onNewProjectRequested);
    connect(m_transportDock, &TransportDock::audioTrackRequested, this, &MainWindow::onAudioTrackRequested);
    connect(m_transportDock, &TransportDock::midiTrackRequested, this, &MainWindow::onMidiTrackRequested);
    connect(m_transportDock, &TransportDock::loadAudioFileRequested, this, &MainWindow::onLoadAudioFileRequested);
    
    // Connect audio engine to transport dock
    connect(m_transportDock, &TransportDock::playRequested, m_audioEngine, &AudioEngine::onTransportPlay);
    connect(m_transportDock, &TransportDock::stopRequested, m_audioEngine, &AudioEngine::onTransportStop);
    connect(m_transportDock, &TransportDock::stopAndReturnRequested, m_audioEngine, &AudioEngine::onTransportStopAndReturn);
    connect(m_transportDock, &TransportDock::positionChanged, m_audioEngine, &AudioEngine::onPositionChanged);
    
    // Connect audio engine to UI components
    connect(m_audioEngine, &AudioEngine::positionChanged, this, &MainWindow::onAudioEnginePositionChanged);
    connect(m_audioEngine, &AudioEngine::playbackStateChanged, this, &MainWindow::onAudioEnginePlaybackStateChanged);
    
    // Connect timeline and transport dock for position synchronization
    // Use Qt::QueuedConnection to prevent signal loops and crashes
    connect(m_timelineWidget, &TimelineWidget::indicatorPositionChanged, 
            m_transportDock, &TransportDock::setPosition, Qt::QueuedConnection);
    connect(m_transportDock, &TransportDock::positionChanged, 
            m_timelineWidget, &TimelineWidget::setIndicatorPosition, Qt::QueuedConnection);
    
    // Connect audio engine position to timeline
    connect(m_audioEngine, &AudioEngine::positionChanged,
            m_timelineWidget, &TimelineWidget::setIndicatorPosition, Qt::QueuedConnection);
    
    // Setup menu bar
    setupMenuBar();
    
    // Set window properties
    setWindowTitle("Music Production Studio");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onPlayRequested()
{
    qDebug() << "Play requested - delegating to audio engine";
    // Audio engine handles this via direct connection
}

void MainWindow::onStopRequested()
{
    qDebug() << "Stop requested - delegating to audio engine";
    // Audio engine handles this via direct connection
}

void MainWindow::onRecordRequested()
{
    qDebug() << "Record requested";
    // TODO: Implement actual recording functionality
    // For now, just acknowledge the request
}

void MainWindow::onStopAndReturnRequested()
{
    qDebug() << "Stop and return to start requested - delegating to audio engine";
    // Audio engine handles this via direct connection
}

void MainWindow::onPositionChanged(double seconds)
{
    qDebug() << "Position changed to:" << seconds << "seconds - delegating to audio engine";
    // Audio engine handles this via direct connection
}

void MainWindow::onNewProjectRequested()
{
    qDebug() << "New project requested";
    // Clear audio engine
    m_audioEngine->clearAudio();
    
    // Reset timeline position
    m_timelineWidget->setIndicatorPosition(0.0);
    m_transportDock->setPosition(0.0);
    
    qDebug() << "New project created - timeline and audio cleared";
}

void MainWindow::onAudioTrackRequested()
{
    qDebug() << "Audio track requested";
    // TODO: Add new audio track to timeline
    // This should create a new Track object and add it to the TimelineWidget
}

void MainWindow::onLoadAudioFileRequested()
{
    qDebug() << "Load audio file requested from add button";
    qDebug() << "MainWindow state check:";
    qDebug() << "  - m_timelineWidget valid:" << (m_timelineWidget != nullptr);
    qDebug() << "  - m_transportDock valid:" << (m_transportDock != nullptr);
    qDebug() << "  - m_audioEngine valid:" << (m_audioEngine != nullptr);
    
    // Call the same loadAudioFile method used by Ctrl+O
    loadAudioFile();
    
    qDebug() << "onLoadAudioFileRequested completed";
}

void MainWindow::onMidiTrackRequested()
{
    qDebug() << "MIDI track requested";
    // TODO: Implement MIDI track creation
    // This should add a new MIDI track to the timeline
}

void MainWindow::onAudioEnginePositionChanged(double seconds)
{
    // Update transport dock position display
    m_transportDock->setPosition(seconds);
    qDebug() << "Audio engine position changed to:" << seconds << "seconds";
}

void MainWindow::onAudioEnginePlaybackStateChanged(bool isPlaying)
{
    // Update transport dock play/stop button state
    qDebug() << "Audio engine playback state changed to:" << (isPlaying ? "playing" : "stopped");
    
    // TODO: Update transport dock button visual states
    // This could involve changing button icons or colors to reflect current state
}

void MainWindow::setupMenuBar()
{
    // Create File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    // New Project action
    QAction *newProjectAction = new QAction("&New Project", this);
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProjectRequested);
    fileMenu->addAction(newProjectAction);
    
    fileMenu->addSeparator();
    
    // Load Audio File action
    QAction *loadAudioAction = new QAction("&Load Audio File...", this);
    loadAudioAction->setShortcut(QKeySequence::Open);
    connect(loadAudioAction, &QAction::triggered, this, &MainWindow::loadAudioFile);
    fileMenu->addAction(loadAudioAction);
    
    fileMenu->addSeparator();
    
    // Exit action
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
}

void MainWindow::loadAudioFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Load Audio File", "", 
        "Audio Files (*.mp3 *.wav *.m4a *.ogg *.flac *.aac);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        AudioResult result = m_audioEngine->loadAudioFile(fileName);
        if (result.isSuccess()) {
            qDebug() << "Audio engine successfully loaded file, now adding to timeline...";
            
            // Add the loaded audio to the first track in the timeline
            qDebug() << "Calling addAudioItemToTrack with file:" << fileName;
            m_timelineWidget->addAudioItemToTrack(fileName, 0);
            qDebug() << "addAudioItemToTrack completed";
            
            // Reset timeline position when new audio is loaded
            qDebug() << "Resetting timeline and transport positions...";
            m_timelineWidget->setIndicatorPosition(0.0);
            m_transportDock->setPosition(0.0);
            qDebug() << "Position reset completed";
            
            // Show success dialog AFTER all operations are complete
            qDebug() << "About to show success dialog...";
            QMessageBox::information(this, "Audio Loaded", 
                QString("Successfully loaded: %1").arg(QFileInfo(fileName).fileName()));
            qDebug() << "Success dialog closed, loadAudioFile method completing";
        } else {
            QMessageBox::warning(this, "Load Failed", 
                QString("Failed to load audio file:\n%1").arg(result.getErrorMessage()));
            qDebug() << "Failed to load audio file:" << fileName << result.getErrorMessage();
        }
    }
}
