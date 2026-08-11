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
    void processFrame();          // capture + display, runs every tick
    void onInferenceFinished();   // called when async detect() completes

signals:
    void frameReady(const QImage& image);
    void statusChanged(bool connected);
    void angleReady(double angle);

private:
    cv::VideoCapture camera;
    YOLOPoseDetector detector;

    QTimer* frameTimer = nullptr;
    bool running = false;

    // async inference bookkeeping
    QFutureWatcher<std::vector<Detection>> inferenceWatcher;
    bool inferenceBusy = false;
    std::vector<Detection> latestDetections;   // last known results, used for overlay
};

#endif // CAMERA_WORKER_H
