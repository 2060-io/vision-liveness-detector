#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/freetype.hpp>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <random>
#include "gesture_detector.h"
#include "translation_manager.h"

enum class GesturesRequesterSystemStatus {
    IDLE = 1,
    PROCESSING = 2,
    DONE = 3,
    FAILURE = 4
};

class GesturesRequester {
public:
    enum class DebugLevel {
        OFF = 0,
        INFO = 1,
        THREADING = 2
    };

    struct GestureRequest {
        std::string gestureId;
        std::string label;
        double time; // in milliseconds
        bool start_gesture;
        bool take_picture_at_the_end;
        std::string icon_path;
        cv::Mat icon;
    };

    GesturesRequester(int number_of_gestures,
                     GestureDetector* gesture_detector,
                     TranslationManager* translator,
                     DebugLevel debug_level = DebugLevel::OFF);
    
    ~GesturesRequester();

    void cleanup();
    cv::Mat process_image(cv::Mat img, 
                         int show_face = 0,
                         const std::unordered_map<std::string, double>& npoints = {},
                         const std::string& warning_message = "");
    
    void set_gestures_list(const std::vector<GestureDetector::AddResult>& gestures);
    void set_report_alive_callback(std::function<void(bool)> callback);
    void set_ask_to_take_picture_callback(std::function<void()> callback);
    void set_overwrite_text(const std::string& text = "", bool failure = false);

    // Disable copy and move
    GesturesRequester(const GesturesRequester&) = delete;
    GesturesRequester& operator=(const GesturesRequester&) = delete;
    GesturesRequester(GesturesRequester&&) = delete;
    GesturesRequester& operator=(GesturesRequester&&) = delete;

private:
    int number_of_gestures_to_request_;
    DebugLevel debug_level_;
    GestureDetector* gesture_detector_;
    TranslationManager* translator_;
    
    std::string overwrite_text_;
    std::vector<std::string> overwrite_text_log_;
    GesturesRequesterSystemStatus process_status_;
    std::vector<GestureRequest> gestures_to_test_;
    std::vector<GestureDetector::AddResult> gestures_list_;
    
    size_t current_gesture_index_;
    uint64_t current_gesture_started_at_; // milliseconds since epoch
    bool start_time_;
    GestureRequest* current_gesture_request_;
    cv::Ptr<cv::freetype::FreeType2> ft_;
    
    std::function<void(bool)> report_alive_callback_;
    std::function<void()> ask_to_take_picture_callback_;

    void gesture_detected_callback(const std::string& gesture_label);
    void move_to_next_gesture(uint64_t current_time = 0);
    void reset_not_alive();
    void generate_gestures_to_test_list(int number_of_gestures_to_request);
    void start_gestures_sequence();
    
    // Image processing helpers
    void draw_square(cv::Mat& img_out, const std::unordered_map<std::string, double>& npoints);
    cv::Mat pixelate_outside_square(cv::Mat img, const std::unordered_map<std::string, double>& npoints);
    void add_icon_to_image(cv::Mat& image, const cv::Mat& icon, int x_pos, int y_pos, int out_width = 64, int out_height = 64);
    void add_text_to_image(cv::Mat& image, 
                          const std::string& text, 
                          double y_pos_percent = 8.3333,
                          cv::Scalar text_color = cv::Scalar(255, 255, 255),
                          cv::Scalar text_bg_color = cv::Scalar(0, 0, 0));
    
    std::vector<std::string> split_text(const std::string& text, 
                                       int image_width,
                                       int font_face,
                                       double font_scale,
                                       int thickness,
                                       int margin = 10);
    
    void add_text_with_freetype(cv::Ptr<cv::freetype::FreeType2> ft2,
                                cv::Mat& image,
                                const std::string& text,
                                double y_pos_percent=10.0,
                                cv::Scalar text_color=cv::Scalar(255, 255, 255),
                                cv::Scalar text_bg_color= cv::Scalar(0, 0, 0),
                                int fontHeight=30);
    
    std::vector<std::string> split_text_freetype(
                                        cv::Ptr<cv::freetype::FreeType2> ft2,
                                        const std::string& text,
                                        int image_width,
                                        int font_height,
                                        int thickness,
                                        int margin);
    
    std::pair<std::string, cv::Mat> process_requests();
};