#include "yoloposedetector.h"

#include <cmath>
#include <algorithm>

bool YOLOPoseDetector::loadModel(const std::string& modelPath)
{
    try
    {
        net = cv::dnn::readNetFromONNX(modelPath);

        if (net.empty())
        {
            return false;
        }

        // CPU for now
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

        return true;
    }
    catch (const cv::Exception&)
    {
        return false;
    }
}

double YOLOPoseDetector::calculateAngle(
    const cv::Point2f& p1,
    const cv::Point2f& p2,
    const cv::Point2f& p3,
    const cv::Point2f& p4)
{
    cv::Point2f v1 = p2 - p1;
    cv::Point2f v2 = p4 - p3;

    double norm1 = cv::norm(v1);
    double norm2 = cv::norm(v2);

    if (norm1 == 0.0 || norm2 == 0.0)
        return 0.0;

    v1 /= norm1;
    v2 /= norm2;

    double cosine = v1.dot(v2);
    cosine = std::clamp(cosine, -1.0, 1.0);
    double angle = std::acos(cosine);
    return angle * 180.0 / CV_PI;
}


std::vector<Detection> YOLOPoseDetector::detect(const cv::Mat& frame)
{
    std::vector<Detection> detections;

    if (frame.empty() || net.empty())
        return detections;

    // --------------------------------------------------
    // 1. Letterbox image
    // --------------------------------------------------
    float scale = std::min(static_cast<float>(inputWidth) / frame.cols, static_cast<float>(inputHeight) / frame.rows);

    int newWidth = static_cast<int>(frame.cols * scale);
    int newHeight = static_cast<int>(frame.rows * scale);

    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(newWidth, newHeight));

    int padX = (inputWidth - newWidth) / 2;
    int padY = (inputHeight - newHeight) / 2;

    cv::Mat inputImage(inputHeight, inputWidth, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(inputImage(cv::Rect(padX, padY, newWidth, newHeight)));

    // --------------------------------------------------
    // 2. Create YOLO blob
    // --------------------------------------------------
    cv::Mat blob = cv::dnn::blobFromImage(inputImage, 1.0 / 255.0, cv::Size(inputWidth, inputHeight), cv::Scalar(), true, false);
    net.setInput(blob);

    // --------------------------------------------------
    // 3. Run YOLO
    // --------------------------------------------------
    std::vector<cv::Mat> outputs;

    net.forward(outputs, net.getUnconnectedOutLayersNames());
    if (outputs.empty())
        return detections;

    cv::Mat output = outputs[0];

    // YOLO pose output:
    //
    // [1, 4 + number_of_classes + 3 * number_of_keypoints, N]
    //
    // Your model:
    //
    // 4 box values
    // 2 classes
    // 2 keypoints × 3 values
    //
    // = 12 values
    //
    // So we expect approximately:
    //
    // [1, 12, N]

    int channels = output.size[1];
    int numCandidates = output.size[2];

    cv::Mat outputMatrix(channels, numCandidates, CV_32F, output.ptr<float>());
    const int numClasses = 2;
    const int numKeypoints = 2;

    const int keypointStart =
        4 + numClasses;


    // --------------------------------------------------
    // 4. Extract candidates
    // --------------------------------------------------
    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> classIds;
    std::vector<std::vector<Keypoint>> allKeypoints;

    for (int i = 0; i < numCandidates; ++i)
    {
        float cx = outputMatrix.at<float>(0, i);
        float cy = outputMatrix.at<float>(1, i);
        float width = outputMatrix.at<float>(2, i);
        float height = outputMatrix.at<float>(3, i);

        // Find class with highest confidence
        int bestClass = -1;
        float bestScore = 0.0f;

        for (int c = 0; c < numClasses; ++c)
        {
            float score =
                outputMatrix.at<float>(4 + c, i);

            if (score > bestScore)
            {
                bestScore = score;
                bestClass = c;
            }
        }

        if (bestScore < confidenceThreshold)
            continue;

        // --------------------------------------------------
        // Bounding box in model coordinates
        // --------------------------------------------------

        float x = cx - width / 2.0f;
        float y = cy - height / 2.0f;

        // Convert back to original camera coordinates
        x = (x - padX) / scale;
        y = (y - padY) / scale;

        width /= scale;
        height /= scale;

        cv::Rect box(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height));
        box &= cv::Rect(0, 0, frame.cols, frame.rows);

        // --------------------------------------------------
        // Keypoints
        // --------------------------------------------------

        std::vector<Keypoint> keypoints;

        for (int k = 0; k < numKeypoints; ++k)
        {
            int index = keypointStart + k * 3;

            float kpX = outputMatrix.at<float>(index, i);
            float kpY =outputMatrix.at<float>(index + 1, i);
            float kpConfidence = outputMatrix.at<float>(index + 2, i);


            // Convert from model coordinates
            // back to camera coordinates
            kpX = (kpX - padX) / scale;
            kpY = (kpY - padY) / scale;
            keypoints.push_back({cv::Point2f(kpX, kpY), kpConfidence});
        }

        boxes.push_back(box);
        scores.push_back(bestScore);
        classIds.push_back(bestClass);
        allKeypoints.push_back(keypoints);
    }

    // --------------------------------------------------
    // 5. Non-Maximum Suppression
    // --------------------------------------------------

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, confidenceThreshold, 0.45f, indices);

    // --------------------------------------------------
    // 6. Create final detections
    // --------------------------------------------------

    for (int index : indices)
    {
        Detection detection;

        detection.classId = classIds[index];
        detection.confidence = scores[index];
        detection.box = boxes[index];
        detection.keypoints = allKeypoints[index];

        detections.push_back(detection);
    }

    return detections;
}


void YOLOPoseDetector::drawResults(
    cv::Mat& frame,
    const std::vector<Detection>& detections)
{
    cv::Point2f wire1;
    cv::Point2f wire2;

    cv::Point2f axis1;
    cv::Point2f axis2;

    bool wireFound = false;
    bool axisFound = false;

    for (const Detection& detection : detections)
    {
        if (detection.keypoints.size() < 2)
            continue;

        cv::Point2f p1 =
            detection.keypoints[0].point;

        cv::Point2f p2 =
            detection.keypoints[1].point;

        // Class 0 = Wire
        if (detection.classId == 0)
        {
            wire1 = p1;
            wire2 = p2;
            wireFound = true;

            cv::line(frame, wire1, wire2, cv::Scalar(0, 255, 0), 3);
            cv::circle(frame, wire1, 6, cv::Scalar(0, 255, 0), -1);
            cv::circle(frame, wire2, 6, cv::Scalar(0, 255, 0), -1);
        }

        // Class 1 = Axis
        else if (detection.classId == 1)
        {
            axis1 = p1;
            axis2 = p2;
            axisFound = true;

            cv::line(frame, axis1, axis2, cv::Scalar(255, 0, 0), 3);
            cv::circle(frame, axis1, 6, cv::Scalar(255, 0, 0), -1);
            cv::circle(frame, axis2, 6, cv::Scalar(255, 0, 0), -1);
        }
    }

    // --------------------------------------------------
    // Calculate angle
    // --------------------------------------------------
    if (wireFound && axisFound)
    {
        double angle = calculateAngle(wire1, wire2, axis1, axis2);
        std::string text = "Angle: " + std::to_string(angle).substr(0, 5) + " deg";
        cv::putText(frame, text, cv::Point(30, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
    }
}
