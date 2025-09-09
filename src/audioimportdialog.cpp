#include "audioimportdialog.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>

// Predefined colors for audio items
const QList<QColor> AudioImportDialog::s_defaultColors = {
    QColor(255, 107, 107), // Red
    QColor(255, 159, 67),  // Orange
    QColor(255, 206, 84),  // Yellow
    QColor(72, 219, 251),  // Light Blue
    QColor(116, 185, 255), // Blue
    QColor(162, 155, 254), // Purple
    QColor(223, 230, 233), // Light Gray
    QColor(255, 118, 117), // Pink
    QColor(85, 239, 196),  // Green
    QColor(129, 236, 236)  // Cyan
};

AudioImportDialog::AudioImportDialog(const QString& filePath, int totalTracks, QWidget* parent)
    : QDialog(parent)
    , m_filePath(filePath)
    , m_totalTracks(totalTracks)
    , m_selectedColor(s_defaultColors.first())
    , m_selectedTrack(0)
{
    setWindowTitle("Import Audio File");
    setModal(true);
    setFixedSize(400, 300);
    
    setupUI();
    updateColorButton();
}

AudioImportDialog::ImportSettings AudioImportDialog::getImportSettings() const
{
    ImportSettings settings;
    settings.targetTrack = m_selectedTrack;
    settings.itemColor = m_selectedColor;
    settings.filePath = m_filePath;
    return settings;
}

void AudioImportDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // File information section
    QGroupBox* fileGroup = new QGroupBox("File Information");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    
    QFileInfo fileInfo(m_filePath);
    QString fileName = fileInfo.fileName();
    QString fileSize = QString::number(fileInfo.size() / 1024.0 / 1024.0, 'f', 2) + " MB";
    
    m_fileInfoLabel = new QLabel(QString("<b>File:</b> %1<br><b>Size:</b> %2").arg(fileName, fileSize));
    m_fileInfoLabel->setWordWrap(true);
    fileLayout->addWidget(m_fileInfoLabel);
    
    mainLayout->addWidget(fileGroup);
    
    // Import settings section
    QGroupBox* settingsGroup = new QGroupBox("Import Settings");
    QGridLayout* settingsLayout = new QGridLayout(settingsGroup);
    
    // Track selection
    QLabel* trackLabel = new QLabel("Target Track:");
    m_trackComboBox = new QComboBox();
    for (int i = 0; i < m_totalTracks; ++i) {
        m_trackComboBox->addItem(QString("Track %1").arg(i + 1), i);
    }
    connect(m_trackComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AudioImportDialog::onTrackChanged);
    
    settingsLayout->addWidget(trackLabel, 0, 0);
    settingsLayout->addWidget(m_trackComboBox, 0, 1);
    
    // Color selection
    QLabel* colorLabel = new QLabel("Item Color:");
    
    QHBoxLayout* colorLayout = new QHBoxLayout();
    m_colorButton = new QPushButton("Choose Color");
    m_colorButton->setMinimumHeight(30);
    connect(m_colorButton, &QPushButton::clicked, this, &AudioImportDialog::onColorButtonClicked);
    
    m_colorPreview = new QFrame();
    m_colorPreview->setFixedSize(30, 30);
    m_colorPreview->setFrameStyle(QFrame::Box);
    m_colorPreview->setLineWidth(1);
    
    colorLayout->addWidget(m_colorButton);
    colorLayout->addWidget(m_colorPreview);
    colorLayout->addStretch();
    
    settingsLayout->addWidget(colorLabel, 1, 0);
    settingsLayout->addLayout(colorLayout, 1, 1);
    
    mainLayout->addWidget(settingsGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("Cancel");
    m_okButton = new QPushButton("Import");
    m_okButton->setDefault(true);
    
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Style the OK button
    m_okButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3d8b40;"
        "}"
    );
}

void AudioImportDialog::onColorButtonClicked()
{
    QColorDialog colorDialog(m_selectedColor, this);
    colorDialog.setWindowTitle("Select Audio Item Color");
    
    // Add custom colors (our predefined palette)
    for (int i = 0; i < s_defaultColors.size(); ++i) {
        colorDialog.setCustomColor(i, s_defaultColors[i]);
    }
    
    if (colorDialog.exec() == QDialog::Accepted) {
        m_selectedColor = colorDialog.selectedColor();
        updateColorButton();
    }
}

void AudioImportDialog::onTrackChanged(int index)
{
    m_selectedTrack = m_trackComboBox->itemData(index).toInt();
}

void AudioImportDialog::updateColorButton()
{
    // Update color preview
    QString colorStyle = QString("background-color: %1; border: 1px solid #ccc;")
                        .arg(m_selectedColor.name());
    m_colorPreview->setStyleSheet(colorStyle);
    
    // Update button text to show color name
    QString colorName = m_selectedColor.name().toUpper();
    m_colorButton->setText(QString("Color: %1").arg(colorName));
}
