#ifndef YOLOPOSEDETECTOR_H
#define YOLOPOSEDETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>

struct Keypoint
{
    cv::Point2f point;
    float confidence;
};

struct Detection
{
    int classId;
    float confidence;
    cv::Rect box;
    std::vector<Keypoint> keypoints;
};

class YOLOPoseDetector
{
public:

    bool loadModel(const std::string& modelPath);
    std::vector<Detection> detect(const cv::Mat& frame);
    void drawResults(cv::Mat& frame, const std::vector<Detection>& detections);
    static double calculateAngle(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3, const cv::Point2f& p4);

private:

    cv::dnn::Net net;

    float confidenceThreshold = 0.25f;
    float keypointThreshold = 0.25f;

    int inputWidth = 640;
    int inputHeight = 640;
};



#endif // YOLOPOSEDETECTOR_H
