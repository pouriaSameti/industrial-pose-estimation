#ifndef CAMERA_WORKER_H
#define CAMERA_WORKER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QFutureWatcher>

#include <opencv2/opencv.hpp>
#include "yoloposedetector.h"

class CameraWorker : public QObject
{
    Q_OBJECT

public:
    explicit CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

public slots:
    void startCamera(const QString& cameraIP);
    void stopCamera();
    void processFrame();
    void onInferenceFinished();

signals:
    void frameReady(const QImage& image);
    void statusChanged(bool connected);
    void angleReady(double angle, const QString& direction);

private:
    cv::VideoCapture camera;
    YOLOPoseDetector detector;

    int frameWidth = 0;

    QTimer* frameTimer = nullptr;
    bool running = false;

    // async inference bookkeeping
    QFutureWatcher<std::vector<Detection>> inferenceWatcher;
    bool inferenceBusy = false;
    std::vector<Detection> latestDetections;
};

#endif // CAMERA_WORKER_H
