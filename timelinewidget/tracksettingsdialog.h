#ifndef TRACKSETTINGSDIALOG_H
#define TRACKSETTINGSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QDial>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include "track.h"

class TrackSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrackSettingsDialog(Track* track, QWidget* parent = nullptr);

private slots:
    void onVolumeChanged(int value);
    void onPanChanged(int value);
    void onTrackNameChanged();
    void onAddEffectClicked();
    void onRemoveEffectClicked();
    void onEffectSelectionChanged();

private:
    void setupUI();
    QGroupBox* createTrackInfoGroup();
    QGroupBox* createMixerGroup();
    QGroupBox* createEffectsGroup();
    QHBoxLayout* createButtonLayout();
    void updateVolumeLabel(int value);
    void updatePanLabel(int value);
    void populateAvailableEffects();
    void applyChanges();
    
    Track* m_track;
    
    // Track Info
    QLineEdit* m_trackNameEdit;
    QComboBox* m_trackColorCombo;
    
    // Mixer Controls
    QSlider* m_volumeSlider;
    QLabel* m_volumeLabel;
    QDial* m_panDial;
    QLabel* m_panLabel;
    QPushButton* m_muteButton;
    QPushButton* m_soloButton;
    
    // Effects Chain
    QListWidget* m_effectsList;
    QComboBox* m_availableEffects;
    QPushButton* m_addEffectButton;
    QPushButton* m_removeEffectButton;
    
    // Dialog Buttons
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
};

#endif // TRACKSETTINGSDIALOG_H
