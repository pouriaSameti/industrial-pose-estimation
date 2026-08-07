#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QImage>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cameraTimer = new QTimer(this);
    connect(cameraTimer, &QTimer::timeout, this, &MainWindow::updateCamera);
    connect(ui->connectCameraButton, &QPushButton::clicked, this, &MainWindow::connectCamera);
}

MainWindow::~MainWindow()
{
    if (cameraTimer->isActive())
        cameraTimer->stop();

    if (camera.isOpened())
        camera.release();

    delete ui;
}


void MainWindow::connectCamera()
{
    QString cameraIP = ui->cameraLineEdit->text();

    if(cameraIP.isEmpty())
    {
        qDebug() << "Camera address is empty";
        return;
    }

    if(camera.isOpened())
    {
        camera.release();
    }


    bool opened = camera.open(cameraIP.toStdString());

    if(!opened)
    {
        qDebug() << "Cannot open camera:"
                 << cameraIP;

        return;
    }

    qDebug() << "Camera connected:"
             << cameraIP;

    cameraTimer->start(30);
}

void MainWindow::updateCamera()
{
    cv::Mat frame;
    camera >> frame;

    if (frame.empty()) {
        qDebug() << "Empty camera frame";
        return;
    }

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage image(frame.data, frame.cols, frame.rows, static_cast<int>(frame.step), QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image);
    ui->cameraLabel->setPixmap(pixmap.scaled(ui->cameraLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
