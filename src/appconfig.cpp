#include "appconfig.h"
#include <QDir>

AppConfig& AppConfig::instance() {
    static AppConfig instance;
    return instance;
}

AppConfig::AppConfig() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    m_settings = new QSettings(configPath + "/music_app.ini", QSettings::IniFormat);
    load();
}

// Audio settings
int AppConfig::getSampleRate() const {
    return m_settings->value("audio/sampleRate", DEFAULT_SAMPLE_RATE).toInt();
}

void AppConfig::setSampleRate(int sampleRate) {
    m_settings->setValue("audio/sampleRate", sampleRate);
}

int AppConfig::getBufferSize() const {
    return m_settings->value("audio/bufferSize", DEFAULT_BUFFER_SIZE).toInt();
}

void AppConfig::setBufferSize(int bufferSize) {
    m_settings->setValue("audio/bufferSize", bufferSize);
}

QString AppConfig::getDefaultAudioPath() const {
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    return m_settings->value("audio/defaultPath", defaultPath).toString();
}

void AppConfig::setDefaultAudioPath(const QString& path) {
    m_settings->setValue("audio/defaultPath", path);
}

// Timeline settings
int AppConfig::getTrackHeight() const {
    return m_settings->value("timeline/trackHeight", DEFAULT_TRACK_HEIGHT).toInt();
}

void AppConfig::setTrackHeight(int height) {
    m_settings->setValue("timeline/trackHeight", height);
}

int AppConfig::getSceneWidth() const {
    return m_settings->value("timeline/sceneWidth", DEFAULT_SCENE_WIDTH).toInt();
}

void AppConfig::setSceneWidth(int width) {
    m_settings->setValue("timeline/sceneWidth", width);
}

int AppConfig::getSceneHeight() const {
    return m_settings->value("timeline/sceneHeight", DEFAULT_SCENE_HEIGHT).toInt();
}

void AppConfig::setSceneHeight(int height) {
    m_settings->setValue("timeline/sceneHeight", height);
}

int AppConfig::getTrackIdWidth() const {
    return m_settings->value("timeline/trackIdWidth", DEFAULT_TRACK_ID_WIDTH).toInt();
}

void AppConfig::setTrackIdWidth(int width) {
    m_settings->setValue("timeline/trackIdWidth", width);
}

// Zoom settings
qreal AppConfig::getZoomFactorX() const {
    return m_settings->value("zoom/factorX", DEFAULT_ZOOM_FACTOR).toReal();
}

void AppConfig::setZoomFactorX(qreal factor) {
    m_settings->setValue("zoom/factorX", factor);
}

qreal AppConfig::getZoomFactorY() const {
    return m_settings->value("zoom/factorY", DEFAULT_ZOOM_FACTOR).toReal();
}

void AppConfig::setZoomFactorY(qreal factor) {
    m_settings->setValue("zoom/factorY", factor);
}

qreal AppConfig::getZoomDelta() const {
    return m_settings->value("zoom/delta", DEFAULT_ZOOM_DELTA).toReal();
}

void AppConfig::setZoomDelta(qreal delta) {
    m_settings->setValue("zoom/delta", delta);
}

void AppConfig::save() {
    m_settings->sync();
}

void AppConfig::load() {
    // Settings are loaded automatically when accessed
    // This method can be used for validation or migration if needed
}
