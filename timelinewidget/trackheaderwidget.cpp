#include "trackheaderwidget.h"
#include <QIcon>
#include <QDebug>

TrackHeaderWidget::TrackHeaderWidget(Track* track, QWidget* parent)
    : QWidget(parent)
    , m_track(track)
    , m_layout(nullptr)
    , m_nameLabel(nullptr)
    , m_muteButton(nullptr)
{
    setupUI();
    styleComponents();
    
    // Set initial values from track
    if (m_track) {
        setTrackName(QString("Track %1").arg(m_track->getIndex() + 1));
        setMuted(m_track->isMuted());
    }
}

void TrackHeaderWidget::setupUI()
{
    // Create horizontal layout for controls
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(8);
    
    // Track name label
    m_nameLabel = new QLabel("Track 1");
    m_nameLabel->setMinimumWidth(80);
    m_nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // Mute button
    m_muteButton = new QPushButton("M");
    m_muteButton->setCheckable(true);
    m_muteButton->setFixedSize(24, 24);
    m_muteButton->setToolTip("Mute Track");
    
    // Connect signals
    connect(m_muteButton, &QPushButton::toggled, this, &TrackHeaderWidget::onMuteButtonToggled);
    
    // Add to layout
    m_layout->addWidget(m_nameLabel);
    m_layout->addStretch(); // Push mute button to the right
    m_layout->addWidget(m_muteButton);
}

void TrackHeaderWidget::styleComponents()
{
    // Style the widget container
    setStyleSheet(
        "TrackHeaderWidget {"
        "    background-color: #2b2b2b;"
        "    border-bottom: 1px solid #404040;"
        "}"
        "TrackHeaderWidget:hover {"
        "    background-color: #353535;"
        "}"
    );
    
    // Style the track name label
    m_nameLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    background: transparent;"
        "    border: none;"
        "}"
    );
    
    // Style the mute button
    m_muteButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #404040;"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "    border-radius: 3px;"
        "    font-weight: bold;"
        "    font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "    border-color: #666666;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #303030;"
        "}"
        "QPushButton:checked {"
        "    background-color: #ff4444;"
        "    border-color: #ff6666;"
        "    color: #ffffff;"
        "}"
        "QPushButton:checked:hover {"
        "    background-color: #ff5555;"
        "}"
    );
}

void TrackHeaderWidget::setTrackName(const QString& name)
{
    if (m_nameLabel) {
        m_nameLabel->setText(name);
    }
}

QString TrackHeaderWidget::getTrackName() const
{
    return m_nameLabel ? m_nameLabel->text() : QString();
}

void TrackHeaderWidget::setMuted(bool muted)
{
    if (m_muteButton) {
        m_muteButton->setChecked(muted);
    }
}

bool TrackHeaderWidget::isMuted() const
{
    return m_muteButton ? m_muteButton->isChecked() : false;
}

void TrackHeaderWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    
    qDebug() << "TrackHeaderWidget: Double-click detected, opening settings for track" << (m_track ? m_track->getIndex() : -1);
    
    if (m_track) {
        emit settingsRequested(m_track);
    }
    
    QWidget::mouseDoubleClickEvent(event);
}

void TrackHeaderWidget::onMuteButtonToggled(bool checked)
{
    qDebug() << "TrackHeaderWidget: Mute button toggled to" << checked;
    
    // Update the track model
    if (m_track) {
        m_track->setMuted(checked);
    }
    
    // Emit signal for timeline widget to handle
    emit muteToggled(checked);
}
