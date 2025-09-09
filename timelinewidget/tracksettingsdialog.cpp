#include "tracksettingsdialog.h"
#include <QApplication>
#include <QDebug>

TrackSettingsDialog::TrackSettingsDialog(Track* track, QWidget* parent)
    : QDialog(parent)
    , m_track(track)
{
    setWindowTitle(QString("Track %1 Settings").arg(track ? track->getIndex() + 1 : 0));
    setModal(true);
    setFixedSize(450, 600);
    
    setupUI();
    
    // Load current track settings
    if (m_track) {
        m_trackNameEdit->setText(QString("Track %1").arg(m_track->getIndex() + 1));
        m_volumeSlider->setValue(static_cast<int>(m_track->getVolume() * 100));
        m_panDial->setValue(static_cast<int>(m_track->getPan() * 50 + 50)); // Convert -1..1 to 0..100
        m_muteButton->setChecked(m_track->isMuted());
        m_soloButton->setChecked(m_track->isSoloed());
        
        updateVolumeLabel(m_volumeSlider->value());
        updatePanLabel(m_panDial->value());
    }
}

void TrackSettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    mainLayout->addWidget(createTrackInfoGroup());
    mainLayout->addWidget(createMixerGroup());
    mainLayout->addWidget(createEffectsGroup());
    mainLayout->addStretch();
    mainLayout->addLayout(createButtonLayout());
}

QGroupBox* TrackSettingsDialog::createTrackInfoGroup()
{
    QGroupBox* group = new QGroupBox("Track Information");
    QGridLayout* layout = new QGridLayout(group);
    
    // Track name
    layout->addWidget(new QLabel("Name:"), 0, 0);
    m_trackNameEdit = new QLineEdit();
    connect(m_trackNameEdit, &QLineEdit::textChanged, this, &TrackSettingsDialog::onTrackNameChanged);
    layout->addWidget(m_trackNameEdit, 0, 1);
    
    // Track color
    layout->addWidget(new QLabel("Color:"), 1, 0);
    m_trackColorCombo = new QComboBox();
    m_trackColorCombo->addItems({"Red", "Orange", "Yellow", "Green", "Blue", "Purple", "Pink", "Cyan"});
    layout->addWidget(m_trackColorCombo, 1, 1);
    
    return group;
}

QGroupBox* TrackSettingsDialog::createMixerGroup()
{
    QGroupBox* group = new QGroupBox("Mixer Controls");
    QGridLayout* layout = new QGridLayout(group);
    
    // Volume fader
    layout->addWidget(new QLabel("Volume:"), 0, 0);
    m_volumeSlider = new QSlider(Qt::Vertical);
    m_volumeSlider->setRange(0, 150); // 0% to 150%
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedHeight(120);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &TrackSettingsDialog::onVolumeChanged);
    
    m_volumeLabel = new QLabel("100%");
    m_volumeLabel->setAlignment(Qt::AlignCenter);
    
    QVBoxLayout* volumeLayout = new QVBoxLayout();
    volumeLayout->addWidget(m_volumeSlider);
    volumeLayout->addWidget(m_volumeLabel);
    layout->addLayout(volumeLayout, 1, 0);
    
    // Pan control
    layout->addWidget(new QLabel("Pan:"), 0, 1);
    m_panDial = new QDial();
    m_panDial->setRange(0, 100); // 0 = full left, 50 = center, 100 = full right
    m_panDial->setValue(50);
    m_panDial->setFixedSize(80, 80);
    connect(m_panDial, &QDial::valueChanged, this, &TrackSettingsDialog::onPanChanged);
    
    m_panLabel = new QLabel("Center");
    m_panLabel->setAlignment(Qt::AlignCenter);
    
    QVBoxLayout* panLayout = new QVBoxLayout();
    panLayout->addWidget(m_panDial);
    panLayout->addWidget(m_panLabel);
    layout->addLayout(panLayout, 1, 1);
    
    // Mute/Solo buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_muteButton = new QPushButton("Mute");
    m_muteButton->setCheckable(true);
    m_muteButton->setStyleSheet(
        "QPushButton:checked { background-color: #ff4444; color: white; }"
    );
    
    m_soloButton = new QPushButton("Solo");
    m_soloButton->setCheckable(true);
    m_soloButton->setStyleSheet(
        "QPushButton:checked { background-color: #ffaa00; color: white; }"
    );
    
    buttonLayout->addWidget(m_muteButton);
    buttonLayout->addWidget(m_soloButton);
    layout->addLayout(buttonLayout, 2, 0, 1, 2);
    
    return group;
}

QGroupBox* TrackSettingsDialog::createEffectsGroup()
{
    QGroupBox* group = new QGroupBox("Effects Chain");
    QVBoxLayout* layout = new QVBoxLayout(group);
    
    // Effects list
    m_effectsList = new QListWidget();
    m_effectsList->setMaximumHeight(120);
    connect(m_effectsList, &QListWidget::itemSelectionChanged, this, &TrackSettingsDialog::onEffectSelectionChanged);
    layout->addWidget(m_effectsList);
    
    // Add/Remove controls
    QHBoxLayout* effectsControlLayout = new QHBoxLayout();
    
    m_availableEffects = new QComboBox();
    populateAvailableEffects();
    
    m_addEffectButton = new QPushButton("Add Effect");
    connect(m_addEffectButton, &QPushButton::clicked, this, &TrackSettingsDialog::onAddEffectClicked);
    
    m_removeEffectButton = new QPushButton("Remove Effect");
    m_removeEffectButton->setEnabled(false);
    connect(m_removeEffectButton, &QPushButton::clicked, this, &TrackSettingsDialog::onRemoveEffectClicked);
    
    effectsControlLayout->addWidget(m_availableEffects);
    effectsControlLayout->addWidget(m_addEffectButton);
    effectsControlLayout->addWidget(m_removeEffectButton);
    
    layout->addLayout(effectsControlLayout);
    
    return group;
}

QHBoxLayout* TrackSettingsDialog::createButtonLayout()
{
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_applyButton = new QPushButton("Apply");
    m_cancelButton = new QPushButton("Cancel");
    m_okButton = new QPushButton("OK");
    m_okButton->setDefault(true);
    
    connect(m_applyButton, &QPushButton::clicked, this, &TrackSettingsDialog::applyChanges);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okButton, &QPushButton::clicked, [this]() {
        applyChanges();
        accept();
    });
    
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    return buttonLayout;
}

void TrackSettingsDialog::onVolumeChanged(int value)
{
    updateVolumeLabel(value);
    if (m_track) {
        m_track->setVolume(value / 100.0f);
    }
}

void TrackSettingsDialog::onPanChanged(int value)
{
    updatePanLabel(value);
    if (m_track) {
        // Convert 0..100 to -1..1
        float panValue = (value - 50) / 50.0f;
        m_track->setPan(panValue);
    }
}

void TrackSettingsDialog::onTrackNameChanged()
{
    // Track name changes could be applied in real-time or on dialog accept
    qDebug() << "Track name changed to:" << m_trackNameEdit->text();
}

void TrackSettingsDialog::onAddEffectClicked()
{
    QString effectName = m_availableEffects->currentText();
    if (!effectName.isEmpty()) {
        m_effectsList->addItem(effectName);
        qDebug() << "Added effect:" << effectName;
    }
}

void TrackSettingsDialog::onRemoveEffectClicked()
{
    int currentRow = m_effectsList->currentRow();
    if (currentRow >= 0) {
        QListWidgetItem* item = m_effectsList->takeItem(currentRow);
        if (item) {
            qDebug() << "Removed effect:" << item->text();
            delete item;
        }
    }
}

void TrackSettingsDialog::onEffectSelectionChanged()
{
    m_removeEffectButton->setEnabled(m_effectsList->currentItem() != nullptr);
}

void TrackSettingsDialog::updateVolumeLabel(int value)
{
    m_volumeLabel->setText(QString("%1%").arg(value));
}

void TrackSettingsDialog::updatePanLabel(int value)
{
    if (value < 45) {
        m_panLabel->setText(QString("L%1").arg(50 - value));
    } else if (value > 55) {
        m_panLabel->setText(QString("R%1").arg(value - 50));
    } else {
        m_panLabel->setText("Center");
    }
}

void TrackSettingsDialog::populateAvailableEffects()
{
    m_availableEffects->addItems({
        "Reverb",
        "Delay",
        "Chorus",
        "Flanger",
        "Phaser",
        "Distortion",
        "Overdrive",
        "Compressor",
        "Limiter",
        "EQ - 3 Band",
        "EQ - Parametric",
        "High Pass Filter",
        "Low Pass Filter",
        "Noise Gate"
    });
}

void TrackSettingsDialog::applyChanges()
{
    if (!m_track) return;
    
    // Apply all changes to the track
    m_track->setMuted(m_muteButton->isChecked());
    m_track->setSoloed(m_soloButton->isChecked());
    
    qDebug() << "Applied track settings changes";
}
