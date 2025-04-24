#include "liveness_detector.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <stdexcept>

// For OpenCV functions
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// ----------------------------------------------------------------
// A dummy implementation of find_face.
// In a real system this would run an actual face detector. (Not implemented yet)
namespace {
std::vector<cv::Rect> find_face(const cv::Mat& img) {
    std::vector<cv::Rect> faces;
    if (!img.empty()) {
        // Dummy: if image is not empty, return one face rectangle.
        faces.push_back(cv::Rect(10, 10, 100, 100));
    }
    return faces;
}
} // end anonymous namespace

// --------------------- Utility method -------------------------
std::string LivenessDetector::drawValueOf(const std::string& name, double value,
                                            double minValue, double maxValue,
                                            int maxCharacters, int decimalPlaces) const {
    double normalized_value = (value - minValue) / (maxValue - minValue);
    int num_hashes = static_cast<int>(normalized_value * maxCharacters);
    if (num_hashes > maxCharacters) {
        num_hashes = maxCharacters;
    }
    std::string result(num_hashes, '#');
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimalPlaces) << value;
    std::string formatted_value = oss.str();
    return name + " " + formatted_value + " " + result;
}

// --------------------- Debug print helper ---------------------
void LivenessDetector::_debug_print(int level_required, const std::string& message) const {
    if (debug_level_ >= level_required) {
        std::cout << message << std::endl;
    }
}

// --------------------- Constructor ------------------------------
LivenessDetector::LivenessDetector(const std::string& verification_token,
                                   const std::string& rd,
                                   const std::string& d,
                                   const std::string& q,
                                   const std::string& lang,
                                   int number_of_gestures_to_request,
                                   int debug_level)
    : verification_token_(verification_token),
      done_(false),
      debug_level_(debug_level),
      glasses_(false),
      frames_counter_(0),
      cfg_(),
      take_a_picture_(false),
      match_counter_(0)
{
    _debug_print(DEBUG_INFO, "LivenessDetector: Constructor called");

    // Instantiate the translator (using locales in "./gestures/locales/")
    translator_ = std::make_unique<TranslationManager>(lang, "./gestures/locales/");

    // Create the face processor with a default model path.
    faceProcessor_ = std::make_unique<FaceProcessor>("default_model_path");
    // Set callback on the face processor.
    faceProcessor_->SetCallback([this](const std::map<std::string, float>& blendshapes,
                                       const std::map<std::string, float>& transformationValues) {
        this->face_processor_result_all_callback(blendshapes, transformationValues);
    });

    // Create the gesture detector.
    gesture_detector_ = std::make_unique<GestureDetector>();

    // Create the gestures requester (using THREADING debug level here).
    gestures_requester_ = std::make_unique<GesturesRequester>(number_of_gestures_to_request,
                                                              gesture_detector_.get(),
                                                              translator_.get(),
                                                              GesturesRequester::DebugLevel::THREADING);
    gestures_requester_->set_report_alive_callback([this](bool alive) {
        this->gestures_requester_result_callback(alive);
    });
    gestures_requester_->set_ask_to_take_picture_callback([this]() {
        this->gestures_requester_take_a_picture_callback();
    });

    // MediaManager initialization (commented out)
    // auto mm_settings = generate_mm_settings(rd, d, q);
    // mediaManager_ = std::make_unique<MediaManager>(mm_settings);

    // Scan the "./gestures" directory for .json files.
    namespace fs = std::filesystem;
    std::vector<fs::path> gesture_files;
    try {
        for (const auto& entry : fs::directory_iterator("./gestures")) {
            if (entry.path().extension() == ".json") {
                gesture_files.push_back(entry.path());
            }
        }
    } catch (const std::exception& e) {
        _debug_print(DEBUG_INFO, std::string("Error reading gestures directory: ") + e.what());
    }
    
    for (const auto& file : gesture_files) {
        _debug_print(DEBUG_INFO, "Gesture file: " + file.string());
        auto addResult = gesture_detector_->add_gesture_from_file(file.string());
        if (!addResult.success) {
            if (debug_level_ >= DEBUG_INFO) {
                _debug_print(DEBUG_INFO, "!!!!!!!!!!!!!!!!!!!!!!!!!!!! " + file.string() + " gesture not added");
            }
        } else {
            // Load the icon image.
            cv::Mat icon = cv::imread(addResult.icon_path, cv::IMREAD_UNCHANGED);
            if (!icon.empty() && icon.channels() == 3) {
                cv::cvtColor(icon, icon, cv::COLOR_BGR2BGRA);
            }
            // In the Python version the icon was appended.
            // Here we simply push the AddResult (which does not include the cv::Mat icon).
            gestures_list_.push_back(addResult);
        }
    }
    gestures_requester_->set_gestures_list(gestures_list_);
    // DeepFace.build_model("VGG-Face") commented out.
    match_counter_ = 0;
}

// --------------------- Destructor -------------------------------
LivenessDetector::~LivenessDetector() {
    _debug_print(DEBUG_INFO, "LivenessDetector Destroyed");
    cleanup();
}

// --------------------- is_done accessor -------------------------
bool LivenessDetector::is_done() const {
    return done_;
}

// --------------------- cleanup method ---------------------------
void LivenessDetector::cleanup() {
    _debug_print(DEBUG_INFO, "LivenessDetector Cleanup");
    if (faceProcessor_) {
        faceProcessor_->SetDoProcessImage(false);
        faceProcessor_.reset();
    }
    if (gesture_detector_) {
        gesture_detector_->cleanup();
        gesture_detector_.reset();
    }
    if (gestures_requester_) {
        gestures_requester_->cleanup();
        gestures_requester_.reset();
    }
    // MediaManager cleanup commented out.
}

// --------------------- process_image ----------------------------
cv::Mat LivenessDetector::process_image(const cv::Mat& img) {
    frames_counter_++;
    int width = img.cols;
    int height = img.rows;
    cv::Mat resized_img;
    if (width > height) {
        int new_height = static_cast<int>(640.0 * height / width);
        cv::resize(img, resized_img, cv::Size(640, new_height));
    } else {
        int new_width = static_cast<int>(480.0 * width / height);
        cv::resize(img, resized_img, cv::Size(new_width, 480));
    }

    faceProcessor_->ProcessImage(resized_img);
    std::string warning_text = _verify_correct_face();

    cv::Mat img_out = gestures_requester_->process_image(resized_img, cfg_.show_face, face_square_normalized_points_, warning_text);
    if (take_a_picture_) {
        take_a_picture_ = false;
        cv::Mat picture = resized_img.clone();
        pictures_.push_back(picture);
    }
    return img_out;
}

// --------------------- face_processor_result_callback -----------
void LivenessDetector::face_processor_result_callback(const std::map<std::string, float>& blendshapes,
                                                      const std::map<std::string, float>& transformationValues) {
    _debug_print(DEBUG_INFO, drawValueOf("Eye blikn Right  ", blendshapes.at("Eye blikn Right"), 0.0, 0.8, 30, 10));
    gesture_detector_->process_signal(blendshapes.at("Eye blikn Right"), 2);

    _debug_print(DEBUG_INFO, drawValueOf("jaw Open         ", blendshapes.at("jaw Open"), 0.0, 0.8, 30, 10));
    gesture_detector_->process_signal(blendshapes.at("jaw Open"), 0);

    _debug_print(DEBUG_INFO, drawValueOf("Mouth Smile Right", blendshapes.at("Mouth Smile Right"), 0.0, 0.8, 30, 10));
    gesture_detector_->process_signal(blendshapes.at("Mouth Smile Right"), 1);

    if (transformationValues.count("Yaw") > 0) {
        _debug_print(DEBUG_INFO, "YAW \u2190\u2192:" + std::to_string(transformationValues.at("Yaw")));
        gesture_detector_->process_signal(transformationValues.at("Yaw"), 3);
    }
    if (transformationValues.count("Pitch") > 0) {
        _debug_print(DEBUG_INFO, "PITCH \u2191\u2193:" + std::to_string(transformationValues.at("Pitch")));
        gesture_detector_->process_signal(transformationValues.at("Pitch"), 4);
    }
    if (transformationValues.count("Roll") > 0) {
        _debug_print(DEBUG_INFO, "ROLL \u223C:" + std::to_string(transformationValues.at("Roll")));
        gesture_detector_->process_signal(transformationValues.at("Roll"), 5);
    }
    if (transformationValues.count("Translation Vector") > 0) {
        _debug_print(DEBUG_INFO, "Translation Vector: " + std::to_string(transformationValues.at("Translation Vector")));
    }
}

// --------------------- face_processor_result_all_callback -----------
void LivenessDetector::face_processor_result_all_callback(const std::map<std::string, float>& blendshapes,
                                                          const std::map<std::string, float>& transformationValues) {
    std::stringstream ss;
    ss << "Liveness Detector Thread>>>>>> " << std::this_thread::get_id();
    _debug_print(DEBUG_THREADING, ss.str());

    // Merge the two dictionaries into one.
    std::unordered_map<std::string, double> signals_dictionary;
    for (const auto& pair : blendshapes) {
        signals_dictionary[pair.first] = static_cast<double>(pair.second);
    }
    for (const auto& pair : transformationValues) {
        signals_dictionary[pair.first] = static_cast<double>(pair.second);
    }
    gesture_detector_->process_signals(signals_dictionary);

    // Retrieve the square values.
    bool has_top = (transformationValues.find("Top Square") != transformationValues.end());
    bool has_left = (transformationValues.find("Left Square") != transformationValues.end());
    bool has_right = (transformationValues.find("Right Square") != transformationValues.end());
    bool has_bottom = (transformationValues.find("Bottom Square") != transformationValues.end());
    if (!(has_top && has_left && has_right && has_bottom)) {
        face_square_normalized_points_.clear();
    } else {
        face_square_normalized_points_["topSquare"] = transformationValues.at("Top Square");
        face_square_normalized_points_["leftSquare"] = transformationValues.at("Left Square");
        face_square_normalized_points_["rightSquare"] = transformationValues.at("Right Square");
        face_square_normalized_points_["bottomSquare"] = transformationValues.at("Bottom Square");
    }
}

// --------------------- gestures_requester_result_callback -----------
void LivenessDetector::gestures_requester_result_callback(bool alive) {
    if (alive) {
        _debug_print(DEBUG_INFO, "The person is alive");
        _compare_local_pictures_with_reference();
    } else {
        _debug_print(DEBUG_INFO, "The person is not alive");
        gestures_requester_->set_overwrite_text(translator_->translate("error.not_verified"), true);
        try {
            // mediaManager_->failure(verification_token_, [](){ done_ = true; });
            // For now simply mark done.
            done_ = true;
        } catch (const std::exception& e) {
            _debug_print(DEBUG_INFO, std::string("Error occurred: ") + e.what());
            gestures_requester_->set_overwrite_text(translator_->translate("error.failure_report_error"), true);
        }
        done_ = true;
    }
}

// --------------------- gestures_requester_take_a_picture_callback -----------
void LivenessDetector::gestures_requester_take_a_picture_callback() {
    _debug_print(DEBUG_INFO, "Take a picture");
    take_a_picture_ = true;
}

// --------------------- get_gestures_requester_process_status -----------
//int LivenessDetector::get_gestures_requester_process_status() const {
//    return gestures_requester_->get_process_status();
//}

// --------------------- get_gestures_requester_overwrite_text_log -----------
//std::vector<std::string> LivenessDetector::get_gestures_requester_overwrite_text_log() const {
//    return gestures_requester_->get_overwrite_text_log();
//}

// --------------------- _verify_correct_face ---------------------
std::string LivenessDetector::_verify_correct_face() const {
    std::vector<std::string> required_keys = {"topSquare", "leftSquare", "rightSquare", "bottomSquare"};
    for (const auto& key : required_keys) {
        if (face_square_normalized_points_.find(key) == face_square_normalized_points_.end()) {
            return translator_->translate("warning.face_not_detected_message");
        }
    }
    if (glasses_) {
        return translator_->translate("warning.face_with_glasses_message");
    }
    double top_square = face_square_normalized_points_.at("topSquare");
    double left_square = face_square_normalized_points_.at("leftSquare");
    double right_square = face_square_normalized_points_.at("rightSquare");
    double bottom_square = face_square_normalized_points_.at("bottomSquare");

    double face_width = right_square - left_square;
    double face_height = bottom_square - top_square;
    double face_center_x = (right_square + left_square) / 2.0;
    double face_center_y = (top_square + bottom_square) / 2.0;

    if (!(cfg_.percentage_min_face_width <= face_width && face_width <= cfg_.percentage_max_face_width)) {
        return translator_->translate("warning.wrong_face_width_message");
    }
    if (!(cfg_.percentage_min_face_height <= face_height && face_height <= cfg_.percentage_max_face_height)) {
        return translator_->translate("warning.wrong_face_height_message");
    }
    double allowed_offset = cfg_.percentage_center_allowed_offset;
    if (!(0.5 - allowed_offset <= face_center_x && face_center_x <= 0.5 + allowed_offset)) {
        return translator_->translate("warning.wrong_face_center_message");
    }
    if (!(0.5 - allowed_offset <= face_center_y && face_center_y <= 0.5 + allowed_offset)) {
        return translator_->translate("warning.wrong_face_center_message");
    }
    return "";
}

// --------------------- _face_match_callback_function ----------------
void LivenessDetector::_face_match_callback_function(const std::string& result, const std::string& id) const {
    _debug_print(DEBUG_INFO, "callback result, id=" + id + ", " + result);
}

// --------------------- _async_face_match --------------------------
std::future<FaceMatchResult> LivenessDetector::_async_face_match(const cv::Mat& image1, const cv::Mat& image2) {
    return std::async(std::launch::async, [this, image1, image2]() -> FaceMatchResult {
        // Optionally save images if matcher_save_match_images is enabled (code commented out)
        FaceMatchResult result;
        if (!image1.empty() && !image2.empty()) {
            result.distance = 0.3; // dummy distance (simulate a good match)
        } else {
            result.distance = 1.0;
        }
        return result;
    });
}

// --------------------- _compare_local_pictures_with_reference -----------
void LivenessDetector::_compare_local_pictures_with_reference() {
    // Launch asynchronous comparison on a separate thread.
    std::thread([this]() {
        this->_async_compare_local_pictures_with_reference();
    }).detach();
}

// --------------------- _async_compare_local_pictures_with_reference -----------
void LivenessDetector::_async_compare_local_pictures_with_reference() {
    faceProcessor_->SetDoProcessImage(false);
    try {
        gestures_requester_->set_overwrite_text(translator_->translate("message.getting_reference_images"));
        // Here we would call mediaManager->download_images_from_token(...)
        // For simulation, we use an empty vector to represent reference images.
        std::vector<std::string> reference_images;
        // If error in download or no images found
        if (reference_images.empty()) {
            gestures_requester_->set_overwrite_text(
                translator_->translate("error.no_reference_images") + ", " +
                translator_->translate("error.not_verified"),
                true);
            // mediaManager_->failure(verification_token_, ...);
            done_ = true;
            return;
        } else {
            gestures_requester_->set_overwrite_text(translator_->translate("message.done"));
        }
        
        std::vector<double> distances;
        int p_index = 0;
        for (const auto& reference_image_path : reference_images) {
            std::vector<cv::Rect> faces_ref = find_face(cv::imread(reference_image_path));
            if (faces_ref.size() == 1) {
                _debug_print(DEBUG_INFO, "The reference image (" + reference_image_path +
                             ") meets the criteria for an acceptable face picture.");
            } else {
                _debug_print(DEBUG_INFO, "The reference image (" + reference_image_path +
                             ") doesn't meet the criteria for an acceptable face picture.");
            }
            cv::Mat reference_image = cv::imread(reference_image_path);
            for (const auto& picture : pictures_) {
                p_index++;
                try {
                    _debug_print(DEBUG_INFO, "Processing picture index: " + std::to_string(p_index));
                    std::vector<cv::Rect> faces_pic = find_face(picture);
                    if (faces_pic.size() == 1) {
                        _debug_print(DEBUG_INFO, "The picture meets the criteria for an acceptable face match.");
                    } else {
                        _debug_print(DEBUG_INFO, "The picture doesn't meet the criteria for an acceptable face match.");
                        gestures_requester_->set_overwrite_text(
                            translator_->translate("message.doing_face_match") + " (" +
                            std::to_string(p_index) + " " +
                            translator_->translate("message.of") + " " +
                            std::to_string(pictures_.size()) + "). " +
                            translator_->translate("error.does_not_meet_criteria_for_acceptable_face_match"),
                            true);
                    }
                    gestures_requester_->set_overwrite_text(
                        translator_->translate("message.doing_face_match") + " (" +
                        std::to_string(p_index) + " " +
                        translator_->translate("message.of") + " " +
                        std::to_string(pictures_.size()) + "). ");
                    auto future_result = _async_face_match(picture, reference_image);
                    FaceMatchResult match_result = future_result.get();
                    _debug_print(DEBUG_INFO, "face match result: " + std::to_string(match_result.distance));
                    distances.push_back(match_result.distance);
                } catch (const std::exception& e) {
                    _debug_print(DEBUG_INFO, std::string("Error occurred in face match: ") + e.what());
                    gestures_requester_->set_overwrite_text(translator_->translate("error.error_doing_face_match"), true);
                }
            }
            // Optionally remove the reference image file here.
            // std::remove(reference_image_path.c_str());
        }
        double total_distance = 0.0;
        for (double d : distances) {
            total_distance += d;
        }
        double average_distance = distances.empty() ? 1.0 : total_distance / distances.size();
        _debug_print(DEBUG_INFO, "Average distance: " + std::to_string(average_distance));
        if (average_distance < 0.4) {
            _debug_print(DEBUG_INFO, "Verified!!");
            gestures_requester_->set_overwrite_text(translator_->translate("message.verified"));
            //TODO: Add this
            //gestures_requester_->set_process_status(static_cast<int>(GesturesRequesterSystemStatus::DONE));
            try {
                // mediaManager_->success(verification_token_, [](){ done_ = true; });
                done_ = true;
            } catch (const std::exception& e) {
                _debug_print(DEBUG_INFO, std::string("Error occurred sending success: ") + e.what());
                gestures_requester_->set_overwrite_text(translator_->translate("error.success_report"), true);
            }
        } else {
            _debug_print(DEBUG_INFO, "Not Verified!!!");
            gestures_requester_->set_overwrite_text(translator_->translate("error.not_verified"), true);
            try {
                // mediaManager_->failure(verification_token_, [](){ done_ = true; });
                done_ = true;
            } catch (const std::exception& e) {
                _debug_print(DEBUG_INFO, std::string("Error occurred sending failure: ") + e.what());
                gestures_requester_->set_overwrite_text(translator_->translate("error.failure_report_error"), true);
            }
        }
    } catch (const std::exception& e) {
        _debug_print(DEBUG_INFO, std::string("Error in _async_compare_local_pictures_with_reference: ") + e.what());
    }
}
