#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ffmpegaudioengine.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class TransportDock;
class TimelineWidget;
class AudioEngine;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPlayRequested();
    void onStopRequested();
    void onRecordRequested();
    void onStopAndReturnRequested();
    void onPositionChanged(double seconds);
    void onNewProjectRequested();
    void onAudioTrackRequested();
    void onMidiTrackRequested();
    void onLoadAudioFileRequested();
    
    // Audio engine slots
    void onAudioEnginePositionChanged(double seconds);
    void onAudioEnginePlaybackStateChanged(bool isPlaying);
    
    // File menu actions
    void loadAudioFile();

private:
    void setupMenuBar();
    Ui::MainWindow *ui;
    TransportDock *m_transportDock;
    TimelineWidget *m_timelineWidget;
    FFmpegAudioEngine *m_audioEngine;
};
#endif // MAINWINDOW_H
