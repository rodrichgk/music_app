#ifndef TRANSPORTDOCK_H
#define TRANSPORTDOCK_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QButtonGroup>
#include <QTimer>
#include <QFrame>
#include "appconfig.h"

class TransportDock : public QWidget
{
    Q_OBJECT

public:
    explicit TransportDock(QWidget *parent = nullptr);
    
    // Transport control methods
    bool isPlaying() const { return m_isPlaying; }
    bool isRecording() const { return m_isRecording; }
    double getCurrentPosition() const { return m_currentPosition; }
    int getBPM() const;
    
    void setPosition(double seconds);
    void setBPM(int bpm);

public slots:
    void play();
    void stop();
    void pause();
    void record();
    void rewind();
    void fastForward();
    void stopAndReturn();
    
    // Project management slots
    void newProject();
    void openProject();
    void saveProject();
    
    // Add button slot
    void showAddMenu();
    
    // Track management
    void addAudioTrack();
    void addMidiTrack();
    void addInstrumentTrack();

signals:
    // Transport signals
    void playRequested();
    void stopRequested();
    void pauseRequested();
    void recordRequested();
    void stopAndReturnRequested();
    void positionChanged(double seconds);
    void bpmChanged(int bpm);
    
    // Project signals
    void newProjectRequested();
    void openProjectRequested();
    void saveProjectRequested();
    
    // Track signals
    void audioTrackRequested();
    void midiTrackRequested();
    void instrumentTrackRequested();
    void loadAudioFileRequested();

private slots:
    void onPlayStopClicked();
    void onRecordClicked();
    void onPositionSliderChanged(int value);
    void onBPMChanged(int bpm);
    void updateTimer();

private:
    void setupUI();
    void setupTransportControls();
    void setupProjectControls();
    void setupTrackControls();
    void setupTimeDisplay();
    void applyModernStyling();
    
    QString formatTime(double seconds) const;
    void updateTimeDisplay();
    
    // Transport state
    bool m_isPlaying;
    bool m_isRecording;
    double m_currentPosition;
    QTimer* m_updateTimer;
    
    // UI Components - Transport
    QFrame* m_transportFrame;
    QPushButton* m_playStopButton;
    QPushButton* m_stopAndReturnButton;
    QPushButton* m_recordButton;
    QPushButton* m_rewindButton;
    QPushButton* m_fastForwardButton;
    QPushButton* m_addButton;
    
    // UI Components - Time/Position
    QFrame* m_timeFrame;
    QLabel* m_timeLabel;
    QSlider* m_positionSlider;
    QSpinBox* m_bpmSpinBox;
    QLabel* m_bpmLabel;
    
    // UI Components - Project
    QFrame* m_projectFrame;
    QToolButton* m_newButton;
    QToolButton* m_openButton;
    QToolButton* m_saveButton;
    
    // UI Components - Track Management
    QFrame* m_trackFrame;
    QToolButton* m_addAudioButton;
    QToolButton* m_addMidiButton;
    QToolButton* m_addInstrumentButton;
    
    // Layout
    QHBoxLayout* m_mainLayout;
};

#endif // TRANSPORTDOCK_H
