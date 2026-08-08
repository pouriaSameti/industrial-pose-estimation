#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <opencv2/opencv.hpp>

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
    void updateCamera();
    void connectCamera();
    void setCameraStatus(bool connected);

private:
    Ui::MainWindow *ui;

    QString cameraIP;
    QTimer *cameraTimer;
    cv::VideoCapture camera;

    bool cameraConnectionStatus = false;
};

#endif // MAINWINDOW_H
