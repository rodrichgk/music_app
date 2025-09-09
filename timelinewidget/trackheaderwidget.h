#ifndef TRACKHEADERWIDGET_H
#define TRACKHEADERWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include "track.h"

class TrackHeaderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrackHeaderWidget(Track* track, QWidget* parent = nullptr);
    
    void setTrackName(const QString& name);
    QString getTrackName() const;
    
    void setMuted(bool muted);
    bool isMuted() const;

signals:
    void muteToggled(bool muted);
    void settingsRequested(Track* track);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onMuteButtonToggled(bool checked);

private:
    void setupUI();
    void styleComponents();
    
    Track* m_track;
    QHBoxLayout* m_layout;
    QLabel* m_nameLabel;
    QPushButton* m_muteButton;
};

#endif // TRACKHEADERWIDGET_H
