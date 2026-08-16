#include "camera_worker.h"

#include <QDebug>
#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

CameraWorker::CameraWorker(QObject *parent)
    : QObject(parent)
{
    QString modelPath = "D:/Computer Vision/Projects/industrial-pose-estimation/4-User Interface/untitled/models/best.onnx";

    if (!detector.loadModel(modelPath.toStdString()))
        qDebug() << "ERROR: Cannot load YOLO model";
    else
        qDebug() << "YOLO model loaded successfully";

    frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, this, &CameraWorker::processFrame);
    connect(&inferenceWatcher, &QFutureWatcher<std::vector<Detection>>::finished, this, &CameraWorker::onInferenceFinished);
}

CameraWorker::~CameraWorker()
{
    stopCamera();
}


void CameraWorker::startCamera(const QString& cameraIP)
{
    qDebug() << "Opening camera:" << cameraIP;

    if (camera.isOpened())
        camera.release();

    bool isDeviceIndex = false;
    int deviceIndex = cameraIP.toInt(&isDeviceIndex);

    if (isDeviceIndex)
    {
#ifdef Q_OS_WIN
        camera.open(deviceIndex, cv::CAP_DSHOW);
#else
        camera.open(deviceIndex, cv::CAP_ANY);
#endif
    }
    else
    {
        camera.open(cameraIP.toStdString());
    }

    if (!camera.isOpened())
    {
        qDebug() << "Cannot open camera";
        running = false;
        emit statusChanged(false);
        return;
    }

    camera.set(cv::CAP_PROP_BUFFERSIZE, 1);
    running = true;
    emit statusChanged(true);
    frameTimer->start(10);
}

void CameraWorker::stopCamera()
{
    running = false;
    frameTimer->stop();

    if (camera.isOpened())
        camera.release();

    emit statusChanged(false);
}

void CameraWorker::processFrame()
{
    if (!running || !camera.isOpened())
        return;

    cv::Mat frame;
    camera >> frame;

    if (frame.empty())
    {
        emit statusChanged(false);
        return;
    }

    frameWidth = frame.cols;
    emit statusChanged(true);

    if (!inferenceBusy)
    {
        inferenceBusy = true;
        cv::Mat frameForInference = frame.clone();
        QFuture<std::vector<Detection>> future = QtConcurrent::run(
            [this, frameForInference]() {
                return detector.detect(frameForInference);
            });
        inferenceWatcher.setFuture(future);
    }


    detector.drawResults(frame, latestDetections);
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage image(frame.data, frame.cols, frame.rows, static_cast<int>(frame.step), QImage::Format_RGB888);
    emit frameReady(image.copy());
}

void CameraWorker::onInferenceFinished()
{
    latestDetections = inferenceWatcher.result();
    inferenceBusy = false;

    bool wireFound = false, axisFound = false;
    cv::Point2f wire1, wire2, axis1, axis2;

    for (const Detection& d : latestDetections)
    {
        if (d.keypoints.size() < 2) continue;

        if (d.classId == 0) { wire1 = d.keypoints[0].point; wire2 = d.keypoints[1].point; axisFound = true; }
        else if (d.classId == 1) { axis1 = d.keypoints[0].point; axis2 = d.keypoints[1].point; wireFound = true; }
    }

    emit wireStatusChanged(wireFound);

    if (wireFound && axisFound)
    {
        double angle = YOLOPoseDetector::calculateSignedAngle(wire1, wire2, axis1, axis2);

        double wireMidX = (axis1.x + axis2.x) / 2.0;
        bool isLeft = wireMidX < (frameWidth / 2.0);

        emit angleReady(angle, isLeft);
    }
}
