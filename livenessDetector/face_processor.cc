#include "face_processor.h"
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <chrono>

// Utility Functions
std::tuple<float, float, float> GetAnglesFromRotationMatrix(const cv::Matx33f& rotation_matrix) {
    float yaw = std::atan2(rotation_matrix(2, 0), rotation_matrix(2, 1));
    float pitch = std::atan2(rotation_matrix(1, 2), std::sqrt(std::pow(rotation_matrix(0, 2), 2) + std::pow(rotation_matrix(2, 2), 2)));
    float roll = std::atan2(rotation_matrix(1, 0), rotation_matrix(0, 0));
    return {yaw, pitch, roll};
}

int64_t current_time_millis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// FaceProcessor Implementation
FaceProcessor::FaceProcessor(const std::string& model_path) {
    auto options = std::make_unique<mediapipe::tasks::vision::face_landmarker::FaceLandmarkerOptions>();
    options->base_options.model_asset_path = model_path;
    options->running_mode = mediapipe::tasks::vision::core::RunningMode::LIVE_STREAM;
    options->num_faces = 1;
    options->min_face_detection_confidence = 0.5f;
    options->min_face_presence_confidence = 0.5f;
    options->min_tracking_confidence = 0.5f;
    options->output_face_blendshapes = true;
    options->output_facial_transformation_matrixes = true;

    options->result_callback = [this](const absl::StatusOr<mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult>& result_or, const mediapipe::Image& image, int64_t timestamp_ms) {
        this->ResultCallbackImpl(result_or, image, timestamp_ms);
    };

    auto face_landmarker_or = mediapipe::tasks::vision::face_landmarker::FaceLandmarker::Create(std::move(options));
    if (!face_landmarker_or.ok()) {
        throw std::runtime_error("Error creating face landmarker: " + std::string(face_landmarker_or.status().message()));
    }
    landmarker_ = std::move(face_landmarker_or.value());
}

FaceProcessor::~FaceProcessor() {
    if (landmarker_) {
        landmarker_->Close();
    }
}

void FaceProcessor::ProcessImage(const cv::Mat& img) {
    if (do_process_image_ && landmarker_) {
        auto input_frame = std::make_shared<mediapipe::ImageFrame>(mediapipe::ImageFormat::SRGB, img.cols, img.rows, mediapipe::ImageFrame::kDefaultAlignmentBoundary);
        cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
        cv::cvtColor(img, input_frame_mat, cv::COLOR_BGR2RGB);
        mediapipe::Image mp_image(input_frame);
        landmarker_->DetectAsync(mp_image, current_time_millis());
    }
}

void FaceProcessor::SetDoProcessImage(bool value) {
    do_process_image_ = value;
}

void FaceProcessor::SetCallback(std::function<void(const std::map<std::string, float>&, const std::map<std::string, float>&)> callbackFn) {
    results_callback_fn_ = std::move(callbackFn);
}

void FaceProcessor::ResultCallbackImpl(const absl::StatusOr<mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult>& result_or, const mediapipe::Image& image, int64_t timestamp_ms) {
    if (!result_or.ok()) {
        std::cerr << "Error processing image: " << result_or.status().message() << std::endl;
        return;
    }
    ProcessResult(result_or.value(), timestamp_ms);
}

void FaceProcessor::ProcessResult(const mediapipe::tasks::vision::face_landmarker::FaceLandmarkerResult& result, int64_t timestamp_ms) {
    std::map<std::string, float> lastBlendshapes;
    std::map<std::string, float> importantTransformationValues;

    if (result.face_blendshapes.has_value()) {
        const auto& blendshapes_list = result.face_blendshapes.value(); 
        if (!blendshapes_list.empty()) {
            const auto& blendshapes = blendshapes_list[0]; 
            for (const auto& category : blendshapes.categories) {
                if (category.category_name.has_value()) {
                    lastBlendshapes[category.category_name.value()] = category.score;
                }
            }
        }
    }

    if (result.facial_transformation_matrixes.has_value()) {
        const auto& transformation_matrix = result.facial_transformation_matrixes.value().at(0);
        cv::Mat matrix(4, 4, CV_32F);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                matrix.at<float>(i, j) = transformation_matrix(i, j);
            }
        }
        cv::Mat rotation_matrix = matrix(cv::Rect(0, 0, 3, 3));
        cv::Vec3f translation_vector = matrix(cv::Rect(3, 0, 1, 3));

        auto [yaw, pitch, roll] = GetAnglesFromRotationMatrix(rotation_matrix);
        importantTransformationValues["Transformation Yaw"] = yaw;
        importantTransformationValues["Transformation Pitch"] = pitch;
        importantTransformationValues["Transformation Roll"] = roll;
        importantTransformationValues["Transformation Translation X"] = translation_vector[0];
        importantTransformationValues["Transformation Translation Y"] = translation_vector[1];
        importantTransformationValues["Transformation Translation Z"] = translation_vector[2];
    }

    if (!result.face_landmarks.empty()) {
        const auto& landmarks = result.face_landmarks[0];
        importantTransformationValues["Top Square"] = landmarks.landmarks.at(10).y;
        importantTransformationValues["Left Square"] = landmarks.landmarks.at(227).x;
        importantTransformationValues["Right Square"] = landmarks.landmarks.at(345).x;
        importantTransformationValues["Bottom Square"] = landmarks.landmarks.at(152).y;
    } else {
        importantTransformationValues["Top Square"] = -1.0f;
        importantTransformationValues["Left Square"] = -1.0f;
        importantTransformationValues["Right Square"] = -1.0f;
        importantTransformationValues["Bottom Square"] = -1.0f;
    }

    if (results_callback_fn_) {
        results_callback_fn_(lastBlendshapes, importantTransformationValues);
    }
}