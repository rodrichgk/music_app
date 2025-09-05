#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QSettings>
#include <QStandardPaths>

class AppConfig {
public:
    static AppConfig& instance();
    
    // Audio settings
    int getSampleRate() const;
    void setSampleRate(int sampleRate);
    
    int getBufferSize() const;
    void setBufferSize(int bufferSize);
    
    QString getDefaultAudioPath() const;
    void setDefaultAudioPath(const QString& path);
    
    // Timeline settings
    int getTrackHeight() const;
    void setTrackHeight(int height);
    
    int getSceneWidth() const;
    void setSceneWidth(int width);
    
    int getSceneHeight() const;
    void setSceneHeight(int height);
    
    int getTrackIdWidth() const;
    void setTrackIdWidth(int width);
    
    // Zoom settings
    qreal getZoomFactorX() const;
    void setZoomFactorX(qreal factor);
    
    qreal getZoomFactorY() const;
    void setZoomFactorY(qreal factor);
    
    qreal getZoomDelta() const;
    void setZoomDelta(qreal delta);
    
    // Save/Load
    void save();
    void load();
    
private:
    AppConfig();
    ~AppConfig() = default;
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
    QSettings* m_settings;
    
    // Default values
    static constexpr int DEFAULT_SAMPLE_RATE = 44100;
    static constexpr int DEFAULT_BUFFER_SIZE = 512;
    static constexpr int DEFAULT_TRACK_HEIGHT = 50;
    static constexpr int DEFAULT_SCENE_WIDTH = 5000;
    static constexpr int DEFAULT_SCENE_HEIGHT = 1020;
    static constexpr int DEFAULT_TRACK_ID_WIDTH = 200;
    static constexpr qreal DEFAULT_ZOOM_FACTOR = 1.0;
    static constexpr qreal DEFAULT_ZOOM_DELTA = 0.1;
};

#endif // APPCONFIG_H
