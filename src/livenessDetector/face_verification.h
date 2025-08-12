#pragma once

#include <map>
#include <string>
#include "translation_manager.h"
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>


// Verifies face bounding box and position, optionally glasses.
std::string verify_correct_face(
    const std::map<std::string, float>& face_square_normalized_points,
    TranslationManager* translator,
    bool glasses = false,
    float percentage_min_face_width = 0.1f,
    float percentage_max_face_width = 0.5f,
    float percentage_min_face_height = 0.1f,
    float percentage_max_face_height = 0.7f,
    float percentage_center_allowed_offset = 0.25f
);

// Haar cascade-based face detector class
class FaceCascadeDetector {
public:
    // Construct with a path to the cascade XML file
    explicit FaceCascadeDetector(const std::string& cascade_path);

    // Returns true if the cascade was loaded correctly
    bool is_loaded() const;

    // Returns true if at least one face found in the image
    // Optionally fills a vector with face rectangles if provided
    bool detect(const cv::Mat& image, std::vector<cv::Rect>* faces = nullptr);

private:
    cv::CascadeClassifier face_cascade_;
    bool loaded_;
};