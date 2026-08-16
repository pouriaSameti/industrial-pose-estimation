#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include "camera_worker.h"
#include "yoloposedetector.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connectCamera();
    void disconnectCamera();
    void displayFrame(const QImage &image);
    void updateAngle(double angle, bool isLeft);
    void setCameraStatus(bool connected);
    void setWireStatus(bool wireDetected);

private:
    Ui::MainWindow *ui;

    QThread *cameraThread;
    CameraWorker *cameraWorker;

    QString cameraIP;
    QTimer *cameraTimer;
    YOLOPoseDetector detector;

    bool cameraConnectionStatus = false;
};


#endif // MAINWINDOW_H
