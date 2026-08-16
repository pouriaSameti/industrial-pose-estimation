#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QMetaObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->connectCameraButton->setCursor(Qt::PointingHandCursor);
    ui->disconnectCameraButton->setCursor(Qt::PointingHandCursor);
    ui->cameraLabel->setPixmap(QPixmap(":/icons/camera.png"));
    setCameraStatus(false);

    // Create worker thread
    cameraThread = new QThread(this);
    cameraWorker = new CameraWorker();
    cameraWorker->moveToThread(cameraThread);

    // Worker -> GUI
    connect(ui->connectCameraButton, &QPushButton::clicked, this, &MainWindow::connectCamera);
    connect(ui->disconnectCameraButton, &QPushButton::clicked, this, &MainWindow::disconnectCamera);
    connect(cameraWorker, &CameraWorker::frameReady, this, &MainWindow::displayFrame);
    connect(cameraWorker, &CameraWorker::statusChanged, this, &MainWindow::setCameraStatus);
    connect(cameraWorker, &CameraWorker::wireDetectionChanged, this, &MainWindow::updateWireStatus);
    connect(cameraWorker, &CameraWorker::angleReady, this, &MainWindow::updateAngle);

    // Start worker thread
    cameraThread->start();
}

MainWindow::~MainWindow()
{
    if (cameraWorker)
        QMetaObject::invokeMethod(cameraWorker, "stopCamera", Qt::BlockingQueuedConnection);

    if (cameraThread)
    {
        cameraThread->quit();
        cameraThread->wait();
    }

    delete cameraWorker;
    delete ui;
}


void MainWindow::connectCamera()
{
    QString cameraIP = ui->cameraLineEdit->text().trimmed();

    if (cameraIP.isEmpty())
    {
        qDebug() << "Camera address is empty";
        setCameraStatus(false);
        return;
    }

    qDebug() << "Connecting to camera:"
             << cameraIP;


    bool isDeviceIndex = false;
    cameraIP.toInt(&isDeviceIndex);

    if (isDeviceIndex)
        ui->sourceLabel->setText(" Webcam");
    else
        ui->sourceLabel->setText(" Remote Camera");

    QMetaObject::invokeMethod(cameraWorker, "startCamera", Qt::QueuedConnection, Q_ARG(QString, cameraIP));
}

void MainWindow::disconnectCamera()
{
    qDebug() << "Disconnecting camera";

    QMetaObject::invokeMethod(cameraWorker, "stopCamera", Qt::QueuedConnection);

    ui->cameraLabel->clear();
    ui->sourceLabel->setText(" None");
    setCameraStatus(false);
}

void MainWindow::displayFrame(const QImage &image)
{
    ui->cameraLabel->setPixmap(QPixmap::fromImage(image).scaled(ui->cameraLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::setCameraStatus(bool connected)
{
    if (connected)
    {

        ui->cameraStatusText->setStyleSheet(
            "QLabel#cameraStatusText {"
            "color: #11d15a;"
            "font-family: 'Segoe UI';"
            "font-size: 10pt;"
            "font-weight: 600;"

            "background-color: #c0edd1;"
            "border-radius: 8px;"
            "}"
            );

        ui->sourceLabel->setStyleSheet(
            "QLabel#sourceLabel {"
            "color: #11d15a;"
            "font-family: 'Segoe UI';"
            "font-size: 10pt;"
            "font-weight: 600;"

            "background-color: #c0edd1;"
            "border-radius: 8px;"
            "}"
            );

        ui->cameraStatusText->setText(" Connected");
        cameraConnectionStatus = true;
    }

    else
    {
        ui->cameraStatusText->setStyleSheet(
            "QLabel#cameraStatusText {"
            "color: #f03d22;"
            "font-family: 'Segoe UI';"
            "font-size: 10pt;"
            "font-weight: 600;"

            "background-color: #edc6c0;"
            "border-radius: 8px;"
            "}"
            );

        ui->sourceLabel->setStyleSheet(
            "QLabel#sourceLabel {"
            "color: #f03d22;"
            "font-family: 'Segoe UI';"
            "font-size: 10pt;"
            "font-weight: 600;"

            "background-color: #edc6c0;"
            "border-radius: 8px;"
            "}"
            );

        cameraConnectionStatus = false;
        ui->cameraStatusText->setText(" Disconnected");
        ui->degreeLabel->setText("0");
        ui->deviationLabel->setText("None");
        ui->cameraLabel->setPixmap(QPixmap(":/icons/camera.png"));
        ui->sourceLabel->setText(" None");
        ui->wireStatusLabel->setText("None");
    }
}


void MainWindow::updateAngle(double angle, bool isLeft)
{
    QString direction = isLeft ? "Left" : "Right";
    ui->deviationLabel->setText(direction);
    ui->degreeLabel->setText(QString::number(std::abs(angle), 'f', 1));
}

void MainWindow::updateWireStatus(bool detected)
{
    if (detected)
        ui->wireStatusLabel->setText("Detected");
    else
        ui->wireStatusLabel->setText("Not Detected");
}
