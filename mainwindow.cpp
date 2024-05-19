#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "timelinewidget.h"
#include <QBoxLayout>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    TimelineWidget *timelineWidget = new TimelineWidget(this);

    // Create a layout for the placeholder widget and add the timeline widget to it
    QVBoxLayout *layout = new QVBoxLayout(ui->centralwidget);
    layout->addWidget(timelineWidget);
    ui->centralwidget->setLayout(layout);
}

MainWindow::~MainWindow()
{
    delete ui;
}
