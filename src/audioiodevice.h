#ifndef AUDIOIODEVICE_H
#define AUDIOIODEVICE_H

#include <QIODevice>
#include <QBuffer>

class FFmpegAudioEngine;

// Custom QIODevice that provides hardware-driven audio streaming
class AudioIODevice : public QIODevice
{
    Q_OBJECT

public:
    explicit AudioIODevice(QBuffer* sourceBuffer, QObject* audioEngine, QObject* parent = nullptr);
    
    // Make bytesAvailable public so FFmpegAudioEngine can access it
    qint64 bytesAvailable() const override;

protected:
    // QIODevice interface - called by audio hardware when it needs data
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;
    bool isSequential() const override { return true; }

private:
    QBuffer* m_sourceBuffer;
    QObject* m_audioEngine;
};

#endif // AUDIOIODEVICE_H
