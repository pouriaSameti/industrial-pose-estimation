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

    bool isDeviceIndex = false;
    int deviceIndex = cameraIP.toInt(&isDeviceIndex);

    if (isDeviceIndex)
    {
        // Plain number (e.g. "0") -> local webcam
#ifdef Q_OS_WIN
        camera.open(deviceIndex, cv::CAP_DSHOW);
#else
        camera.open(deviceIndex, cv::CAP_ANY);
#endif
    }
    else
    {
        // Anything else -> remote stream URL
        camera.open(cameraIP.toStdString(), cv::CAP_FFMPEG);
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
        cv::Mat frameForInference = frame.clone(); // detect() runs on another thread
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

        if (d.classId == 0) { wire1 = d.keypoints[0].point; wire2 = d.keypoints[1].point; wireFound = true; }
        else if (d.classId == 1) { axis1 = d.keypoints[0].point; axis2 = d.keypoints[1].point; axisFound = true; }
    }


    emit wireDetectionChanged(wireFound);

    if (wireFound && axisFound)
    {
        double angle = YOLOPoseDetector::calculateAngle(wire1, wire2, axis1, axis2);

        double dx = wire2.x - wire1.x;
        double dy = wire2.y - wire1.y;

        QString direction;
        if (std::abs(dx) < 1e-6)
        {
            direction = "Direct";
        }
        else
        {
            double slope = dy / dx;
            direction = (slope > 0) ? "Left" : "Right";
        }

        emit angleReady(angle, direction);
    }
}
