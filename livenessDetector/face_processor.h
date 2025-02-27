#ifndef FACE_PROCESSOR_H
#define FACE_PROCESSOR_H

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <opencv2/opencv.hpp>
#include "absl/status/statusor.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/formats/image.h"
#include "mediapipe/tasks/cc/vision/face_landmarker/face_landmarker.h"


std::tuple<float, float, float> GetAnglesFromRotationMatrix(const cv::Matx33f& rotation_matrix);

int64_t current_time_millis();

class FaceProcessor {
public:
    FaceProcessor(const std::string& model_path);
    ~FaceProcessor();

    void ProcessImage(const cv::Mat& img);
    void SetDoProcessImage(bool value);
    void SetCallback(std::function<void(const std::map<std::string, float>&, const std::map<std::string, float>&)> callbackFn);

private:
    void ResultCallbackImpl(const absl::StatusOr<mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult>& result_or, const mediapipe::Image& image, int64_t timestamp_ms);
    void ProcessResult(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult& result, int64_t timestamp_ms);

    std::unique_ptr<mediapipe::tasks::vision::face_landmarker::FaceLandmarker> landmarker_;
    bool do_process_image_ = false;
    std::function<void(const std::map<std::string, float>&, const std::map<std::string, float>&)> results_callback_fn_;
};

#endif // FACE_PROCESSOR_H