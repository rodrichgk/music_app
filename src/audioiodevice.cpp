#include "audioiodevice.h"
#include "ffmpegaudioengine.h"
#include <QDebug>

AudioIODevice::AudioIODevice(QBuffer* sourceBuffer, QObject* audioEngine, QObject* parent)
    : QIODevice(parent)
    , m_sourceBuffer(sourceBuffer)
    , m_audioEngine(audioEngine)
{
    qDebug() << "AudioIODevice: Constructor - source buffer size:" << (sourceBuffer ? sourceBuffer->size() : 0);
    // Open in read-only mode for audio output
    open(QIODevice::ReadOnly);
    qDebug() << "AudioIODevice: Opened in ReadOnly mode, isOpen():" << isOpen();
}

qint64 AudioIODevice::readData(char* data, qint64 maxlen)
{
    // This is called by the audio hardware when it needs data
    qDebug() << "AudioIODevice::readData called - hardware requesting" << maxlen << "bytes";
    
    if (!m_sourceBuffer || !m_sourceBuffer->isOpen()) {
        qDebug() << "AudioIODevice: Source buffer not available or not open";
        return 0;
    }
    
    // Read data from our source buffer
    qint64 bytesRead = m_sourceBuffer->read(data, maxlen);
    qDebug() << "AudioIODevice: Read" << bytesRead << "bytes from source buffer";
    qDebug() << "AudioIODevice: Source buffer position:" << m_sourceBuffer->pos() << "/ size:" << m_sourceBuffer->size();
    
    // Check if we've reached the end
    if (bytesRead == 0 && m_sourceBuffer->atEnd()) {
        qDebug() << "AudioIODevice: Reached end of source buffer - signaling completion";
        // Signal the engine that playback is complete
        QMetaObject::invokeMethod(m_audioEngine, "onPlaybackComplete", Qt::QueuedConnection);
    }
    
    return bytesRead;
}

qint64 AudioIODevice::writeData(const char* data, qint64 len)
{
    // Not used for audio output
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}

qint64 AudioIODevice::bytesAvailable() const
{
    if (!m_sourceBuffer || !m_sourceBuffer->isOpen()) {
        return 0;
    }
    
    qint64 available = m_sourceBuffer->bytesAvailable();
    // Remove excessive logging - only log when there's actually data
    if (available > 0) {
        qDebug() << "AudioIODevice::bytesAvailable() returning:" << available;
    }
    return available;
}
