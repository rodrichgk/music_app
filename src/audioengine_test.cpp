#include "audioengine.h"
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QFileDialog>
#include <QDebug>

class AudioEngineTestWindow : public QMainWindow
{
    Q_OBJECT

public:
    AudioEngineTestWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_audioEngine(new AudioEngine(this))
        , m_positionLabel(new QLabel("Position: 0.0s", this))
        , m_durationLabel(new QLabel("Duration: 0.0s", this))
        , m_statusLabel(new QLabel("Status: Stopped", this))
    {
        setupUI();
        connectSignals();
    }

private slots:
    void loadAudioFile()
    {
        QString fileName = QFileDialog::getOpenFileName(this,
            "Load Audio File", "", "Audio Files (*.mp3 *.wav *.m4a *.ogg)");
        
        if (!fileName.isEmpty()) {
            AudioResult result = m_audioEngine->loadAudioFile(fileName);
            if (result.isSuccess()) {
                m_statusLabel->setText("Status: Audio Loaded");
                qDebug() << "Successfully loaded:" << fileName;
            } else {
                m_statusLabel->setText("Status: Load Failed - " + result.getErrorMessage());
                qDebug() << "Failed to load:" << fileName << result.getErrorMessage();
            }
        }
    }

    void onPositionChanged(double seconds)
    {
        m_positionLabel->setText(QString("Position: %1s").arg(seconds, 0, 'f', 2));
    }

    void onDurationChanged(qint64 duration)
    {
        double seconds = duration / 1000.0;
        m_durationLabel->setText(QString("Duration: %1s").arg(seconds, 0, 'f', 2));
    }

    void onPlaybackStateChanged(bool isPlaying)
    {
        m_statusLabel->setText(QString("Status: %1").arg(isPlaying ? "Playing" : "Stopped"));
    }

private:
    void setupUI()
    {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        // File loading
        QPushButton *loadButton = new QPushButton("Load Audio File", this);
        connect(loadButton, &QPushButton::clicked, this, &AudioEngineTestWindow::loadAudioFile);
        layout->addWidget(loadButton);

        // Transport controls
        QHBoxLayout *transportLayout = new QHBoxLayout();
        
        QPushButton *playButton = new QPushButton("Play", this);
        connect(playButton, &QPushButton::clicked, m_audioEngine, &AudioEngine::play);
        transportLayout->addWidget(playButton);

        QPushButton *pauseButton = new QPushButton("Pause", this);
        connect(pauseButton, &QPushButton::clicked, m_audioEngine, &AudioEngine::pause);
        transportLayout->addWidget(pauseButton);

        QPushButton *stopButton = new QPushButton("Stop", this);
        connect(stopButton, &QPushButton::clicked, m_audioEngine, &AudioEngine::stop);
        transportLayout->addWidget(stopButton);

        layout->addLayout(transportLayout);

        // Status labels
        layout->addWidget(m_positionLabel);
        layout->addWidget(m_durationLabel);
        layout->addWidget(m_statusLabel);

        setWindowTitle("Audio Engine Test");
        resize(400, 200);
    }

    void connectSignals()
    {
        connect(m_audioEngine, &AudioEngine::positionChanged,
                this, &AudioEngineTestWindow::onPositionChanged);
        connect(m_audioEngine, &AudioEngine::durationChanged,
                this, &AudioEngineTestWindow::onDurationChanged);
        connect(m_audioEngine, &AudioEngine::playbackStateChanged,
                this, &AudioEngineTestWindow::onPlaybackStateChanged);
    }

    AudioEngine *m_audioEngine;
    QLabel *m_positionLabel;
    QLabel *m_durationLabel;
    QLabel *m_statusLabel;
};

#include "audioengine_test.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AudioEngineTestWindow window;
    window.show();

    return app.exec();
}
