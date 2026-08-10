#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QDebug>
#include <QImage>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->connectCameraButton->setCursor(Qt::PointingHandCursor);
    setCameraStatus(false);

    cameraTimer = new QTimer(this);
    connect(cameraTimer, &QTimer::timeout, this, &MainWindow::updateCamera);
    connect(ui->connectCameraButton, &QPushButton::clicked, this, &MainWindow::connectCamera);

    QString modelPath = "D:/Computer Vision/Projects/industrial-pose-estimation/4-User Interface/untitled/models/best.onnx";
    qDebug() << "Model path:" << modelPath;

    if (!QFile::exists(modelPath))
    {
        qDebug() << "ERROR: Model file does not exist!";
    }
    else
    {
        qDebug() << "Model file exists.";

        if (!detector.loadModel(modelPath.toStdString()))
        {
            qDebug() << "ERROR: Cannot load YOLO model";
        }
        else
        {
            qDebug() << "YOLO model loaded successfully";
        }
    }
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

    if(cameraIP.isEmpty()){
        qDebug() << "Camera address is empty";
        setCameraStatus(false);
        return;
    }

    if(camera.isOpened())
        camera.release();


    bool opened = camera.open(cameraIP.toStdString());

    if(!opened){
        setCameraStatus(false);
        qDebug() << "Cannot open camera:" << cameraIP;
        return;
    }

    setCameraStatus(true);
    qDebug() << "Camera connected:" << cameraIP;
    cameraTimer->start(30);
}

void MainWindow::updateCamera()
{
    cv::Mat frame;
    camera >> frame;

    if (frame.empty()) {
        qDebug() << "Empty camera frame";
        setCameraStatus(false);
        return;
    }

    setCameraStatus(true);
    std::vector<Detection> detections = detector.detect(frame);
    detector.drawResults(frame, detections);

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage image(frame.data, frame.cols, frame.rows, static_cast<int>(frame.step), QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image);
    ui->cameraLabel->setPixmap(pixmap.scaled(ui->cameraLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
    }
}
