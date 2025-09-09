#ifndef AUDIOIMPORTDIALOG_H
#define AUDIOIMPORTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QColorDialog>
#include <QFrame>
#include <QFileInfo>

class AudioImportDialog : public QDialog
{
    Q_OBJECT

public:
    struct ImportSettings {
        int targetTrack;
        QColor itemColor;
        QString filePath;
    };

    explicit AudioImportDialog(const QString& filePath, int totalTracks, QWidget* parent = nullptr);
    
    ImportSettings getImportSettings() const;

private slots:
    void onColorButtonClicked();
    void onTrackChanged(int index);

private:
    void setupUI();
    void updateColorButton();
    
    // UI Components
    QLabel* m_fileInfoLabel;
    QComboBox* m_trackComboBox;
    QPushButton* m_colorButton;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QFrame* m_colorPreview;
    
    // Data
    QString m_filePath;
    int m_totalTracks;
    QColor m_selectedColor;
    int m_selectedTrack;
    
    // Predefined colors for quick selection
    static const QList<QColor> s_defaultColors;
};

#endif // AUDIOIMPORTDIALOG_H
