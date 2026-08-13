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
    setCameraStatus(false);

    // Create worker thread
    cameraThread = new QThread(this);
    cameraWorker = new CameraWorker();
    cameraWorker->moveToThread(cameraThread);

    // Worker -> GUI
    connect(cameraWorker, &CameraWorker::frameReady, this, &MainWindow::displayFrame);
    connect(cameraWorker, &CameraWorker::statusChanged, this, &MainWindow::setCameraStatus);
    connect(cameraWorker, &CameraWorker::angleReady, this, &MainWindow::updateAngle);

    // Connect button
    connect(ui->connectCameraButton, &QPushButton::clicked, this, &MainWindow::connectCamera);

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


    QMetaObject::invokeMethod(cameraWorker, "startCamera", Qt::QueuedConnection, Q_ARG(QString, cameraIP));
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
            "border-radius: 20px;"
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
            "border-radius: 20px;"
            "}"
            );

        ui->cameraStatusText->setText(" Disconnected");
        cameraConnectionStatus = false;
        ui->degreeLabel->setText("0");
    }
}

void MainWindow::updateAngle(double angle)
{
    ui->degreeLabel->setText(QString::number(angle));
}
