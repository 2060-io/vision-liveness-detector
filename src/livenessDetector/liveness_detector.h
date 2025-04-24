#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <future>
#include <opencv2/opencv.hpp>

#include "gesture.h"
#include "gesture_detector.h"
#include "gestures_requester.h"
#include "translation_manager.h"
#include "face_processor.h"

// A minimal FaceConfig (from liveness_detector_face_config) (For Now)
class FaceConfig {
public:
    // These values are examples. Adjust as needed.
    float percentage_min_face_width = 0.1f;
    float percentage_max_face_width = 1.0f;
    float percentage_min_face_height = 0.1f;
    float percentage_max_face_height = 1.0f;
    float percentage_center_allowed_offset = 0.2f;
    int show_face = 0;
};

// A simple structure to hold a face‐match result.
struct FaceMatchResult {
    double distance;
};

class LivenessDetector {
public:
    // Debug levels (corresponding to Python constants)
    static const int DEBUG_OFF = 0;
    static const int DEBUG_INFO = 1;
    static const int DEBUG_THREADING = 2;

    // Constructor parameters:
    // verification_token, rd, d, q are strings used for mediaManager settings (see Python),
    // but the media manager calls remain commented.
    LivenessDetector(const std::string& verification_token,
                     const std::string& rd,
                     const std::string& d,
                     const std::string& q,
                     const std::string& lang,
                     int number_of_gestures_to_request,
                     int debug_level = DEBUG_OFF);
    ~LivenessDetector();

    bool is_done() const;
    void cleanup();
    cv::Mat process_image(const cv::Mat& img);

    // Callbacks from the face processor
    void face_processor_result_callback(const std::map<std::string, float>& blendshapes,
                                        const std::map<std::string, float>& transformationValues);
    void face_processor_result_all_callback(const std::map<std::string, float>& blendshapes,
                                            const std::map<std::string, float>& transformationValues);

    // Callbacks from the gestures requester
    void gestures_requester_result_callback(bool alive);
    void gestures_requester_take_a_picture_callback();

    // Accessors for requester information
    //int get_gestures_requester_process_status() const;
    //std::vector<std::string> get_gestures_requester_overwrite_text_log() const;

private:
    // Private members
    std::unique_ptr<TranslationManager> translator_;
    std::string verification_token_;
    bool done_;
    int debug_level_;
    bool glasses_;
    size_t frames_counter_;
    FaceConfig cfg_;
    std::unique_ptr<FaceProcessor> faceProcessor_;
    std::unique_ptr<GestureDetector> gesture_detector_;
    std::unique_ptr<GesturesRequester> gestures_requester_;
    // std::unique_ptr<MediaManager> mediaManager_; // mediaManager calls left in comments

    // Gestures list (from add_gesture_from_file)
    std::vector<GestureDetector::AddResult> gestures_list_;

    // Pictures captured
    std::vector<cv::Mat> pictures_;
    bool take_a_picture_;

    // Face square points as normalized values
    std::unordered_map<std::string, double> face_square_normalized_points_;

    size_t match_counter_;

    // Private helper methods
    // Prints message if debug_level_ is high enough.
    void _debug_print(int level_required, const std::string& message) const;
    std::string drawValueOf(const std::string& name, double value, double minValue, double maxValue, int maxCharacters, int decimalPlaces) const;
    std::string _verify_correct_face() const;
    void _face_match_callback_function(const std::string& result, const std::string& id) const;
    std::future<FaceMatchResult> _async_face_match(const cv::Mat& image1, const cv::Mat& image2);
    void _compare_local_pictures_with_reference();
    void _async_compare_local_pictures_with_reference();
};
