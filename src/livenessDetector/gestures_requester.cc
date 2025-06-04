#include "gestures_requester.h"
#include <chrono>
#include <algorithm>
#include <random>
#include <iostream>
#include <filesystem>

using namespace std::chrono;

GesturesRequester::GesturesRequester(int number_of_gestures,
                                     GestureDetector* gesture_detector,
                                     TranslationManager* translator,
                                     const std::string& font_path,
                                     DebugLevel debug_level)
    : number_of_gestures_to_request_(number_of_gestures),
      debug_level_(debug_level),
      gesture_detector_(gesture_detector),
      translator_(translator),
      overwrite_text_(""),
      process_status_(GesturesRequesterSystemStatus::IDLE),
      current_gesture_index_(0),
      current_gesture_started_at_(0),
      start_time_(false),
      current_gesture_request_(nullptr),
      ft_(nullptr) // initialize as nullptr
{
    gesture_detector_->set_signal_trigger_callback([this](const std::string& label) {
        this->gesture_detected_callback(label);
    });

    // Check if the font file exists
    if (std::filesystem::exists(font_path)) {
        try {
            ft_ = cv::freetype::createFreeType2(); // allocate instance
            ft_->loadFontData(font_path, 0);       // try loading font
        } catch (const cv::Exception& e) {
            std::cerr << "Failed to load font: " << font_path << "\n"
                      << e.what() << std::endl;
            ft_ = nullptr; // loading failed
        }
    } else {
        std::cerr << "Font file does not exist: " << font_path << std::endl;
        ft_ = nullptr; // File does not exist
    }
}

GesturesRequester::~GesturesRequester() {
    cleanup();
}

void GesturesRequester::cleanup() {
    gestures_to_test_.clear();
    gestures_list_.clear();
    current_gesture_request_ = nullptr;
}

cv::Mat GesturesRequester::process_image(cv::Mat img, 
                                        int show_face,
                                        const std::unordered_map<std::string, double>& npoints,
                                        const std::string& warning_message) {
    cv::Mat img_out = img.clone();
    
    if (!npoints.empty()) {
        if (show_face == 1) {
            draw_square(img_out, npoints);
        } else if (show_face == 2) {
            img_out = pixelate_outside_square(img_out, npoints);
        }
    }
    
    auto [text, icon] = process_requests();
    if (!overwrite_text_.empty()) {
        text = overwrite_text_;
    }
    
    if (ft_ != nullptr){
        add_text_with_freetype(ft_, img_out, text);
    } else {
        add_text_to_image(img_out, text);
    }
    
    if (!icon.empty()) {
        add_icon_to_image(img_out, icon, 550, 400);
    }
    
    if (!warning_message.empty()) {
        if (ft_ != nullptr){
            add_text_with_freetype(ft_, img_out, warning_message, 80, cv::Scalar(0, 255, 255));
        } else {
            add_text_to_image(img_out, warning_message, 80, cv::Scalar(0, 255, 255));
        }
    }
    
    return img_out;
}

void GesturesRequester::set_gestures_list(const std::vector<GestureDetector::AddResult>& gestures) {
    gestures_list_ = gestures;
    generate_gestures_to_test_list(number_of_gestures_to_request_);
    start_gestures_sequence();
}

void GesturesRequester::gesture_detected_callback(const std::string& gesture_label) {
    std::cout << "Gesture Detected:" << gesture_label << std::endl;
    move_to_next_gesture();
}

void GesturesRequester::move_to_next_gesture(uint64_t current_time) {
    if (current_gesture_index_ == 0) return;

    if (!current_time) {
        auto now = time_point_cast<milliseconds>(system_clock::now());
        current_time = now.time_since_epoch().count();
    }
    
    if (current_gesture_request_->take_picture_at_the_end && ask_to_take_picture_callback_) {
        ask_to_take_picture_callback_();
    }
    
    //std::cout << "move to next gesture: " << current_gesture_index_ << std::endl;
    if (current_gesture_index_ + 2 == gestures_to_test_.size()) {
        //std::cout << "Calling alive" << std::endl;
        if (report_alive_callback_) {
            report_alive_callback_(true);
        }
    }


    current_gesture_index_++;
    if (current_gesture_index_ < gestures_to_test_.size()) {
        current_gesture_request_ = &gestures_to_test_[current_gesture_index_];
        current_gesture_started_at_ = current_time;
        
        if (current_gesture_request_->start_gesture) {
            gesture_detector_->start_by_label(current_gesture_request_->label);
        }
    }
}

void GesturesRequester::generate_gestures_to_test_list(int number_of_gestures_to_request) {
    gestures_to_test_.clear();
    gestures_to_test_.push_back({"notAlive", "Not Alive", 5000, false, false, "", cv::Mat()});
    gestures_to_test_.push_back({"starting", "Starting", 5000, false, false, "", cv::Mat()});

    std::vector<GestureDetector::AddResult> selected_gestures;
    std::sample(gestures_list_.begin(), gestures_list_.end(),
               std::back_inserter(selected_gestures),
               number_of_gestures_to_request,
               std::mt19937{std::random_device{}()});
  
    for (const auto& gesture : selected_gestures) {
        cv::Mat icon;
        if (!gesture.icon_path.empty() && std::filesystem::exists(gesture.icon_path)) {
            icon = cv::imread(gesture.icon_path, cv::IMREAD_UNCHANGED);
        }
        
        gestures_to_test_.push_back({
            gesture.gestureId,
            gesture.label,
            gesture.total_recommended_max_time,
            true,
            gesture.take_picture_at_the_end,
            gesture.icon_path,
            icon
        });
    }
    
    gestures_to_test_.push_back({"youarealive", "You Are Alive", 5000, false, false, "", cv::Mat()});
}

void GesturesRequester::start_gestures_sequence() {
    current_gesture_index_ = 1;
    current_gesture_request_ = &gestures_to_test_[current_gesture_index_];
    start_time_ = true;
}

// Image processing helper methods implementation
void GesturesRequester::draw_square(cv::Mat& img_out, const std::unordered_map<std::string, double>& npoints) {
    int height = img_out.rows;
    int width = img_out.cols;

    int top = static_cast<int>(npoints.at("topSquare") * height);
    int left = static_cast<int>(npoints.at("leftSquare") * width);
    int right = static_cast<int>(npoints.at("rightSquare") * width);
    int bottom = static_cast<int>(npoints.at("bottomSquare") * height);

    cv::rectangle(img_out, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 255, 0), 2);
}

cv::Mat GesturesRequester::pixelate_outside_square(cv::Mat img, const std::unordered_map<std::string, double>& npoints) {
    int height = img.rows;
    int width = img.cols;

    int top = static_cast<int>(npoints.at("topSquare") * height);
    int left = static_cast<int>(npoints.at("leftSquare") * width);
    int right = static_cast<int>(npoints.at("rightSquare") * width);
    int bottom = static_cast<int>(npoints.at("bottomSquare") * height);

    cv::Mat mask = cv::Mat::zeros(height, width, CV_8U);
    cv::rectangle(mask, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(255), cv::FILLED);

    cv::Mat pixelated;
    cv::resize(img, pixelated, cv::Size(width/10, height/10));
    cv::resize(pixelated, pixelated, cv::Size(width, height), 0, 0, cv::INTER_NEAREST);

    cv::Mat result;
    img.copyTo(result, mask);
    pixelated.copyTo(result, ~mask);

    return result;
}

void GesturesRequester::add_icon_to_image(cv::Mat& image, const cv::Mat& icon, int x_pos, int y_pos, int out_width, int out_height) {
    if (icon.empty()) return;
    
    cv::Mat resized_icon;
    cv::resize(icon, resized_icon, cv::Size(out_width, out_height));
    
    if (resized_icon.channels() == 4) {
        std::vector<cv::Mat> channels;
        cv::split(resized_icon, channels);
        cv::Mat alpha = channels[3];
        channels.pop_back();
        cv::merge(channels, resized_icon);
        
        cv::Mat roi = image(cv::Rect(x_pos, y_pos, out_width, out_height));
        resized_icon.copyTo(roi, alpha);
    } else {
        resized_icon.copyTo(image(cv::Rect(x_pos, y_pos, out_width, out_height)));
    }
}

std::vector<std::string> GesturesRequester::split_text(const std::string& text, 
                                                      int image_width,
                                                      int font_face,
                                                      double font_scale,
                                                      int thickness,
                                                      int margin) {
    std::vector<std::string> lines;
    std::vector<std::string> input_lines;
    size_t start = 0;
    size_t end = text.find('\n');
    
    while (end != std::string::npos) {
        input_lines.push_back(text.substr(start, end - start));
        start = end + 1;
        end = text.find('\n', start);
    }
    input_lines.push_back(text.substr(start));

    int max_width = image_width - 2 * margin;
    
    for (const auto& line : input_lines) {
        cv::Size text_size = cv::getTextSize(line, font_face, font_scale, thickness, nullptr);
        if (text_size.width <= max_width) {
            lines.push_back(line);
            continue;
        }
        
        std::vector<std::string> words;
        size_t word_start = 0;
        size_t space_pos = line.find(' ');
        while (space_pos != std::string::npos) {
            words.push_back(line.substr(word_start, space_pos - word_start));
            word_start = space_pos + 1;
            space_pos = line.find(' ', word_start);
        }
        words.push_back(line.substr(word_start));
        
        std::string current_line;
        for (const auto& word : words) {
            std::string test_line = current_line.empty() ? word : current_line + " " + word;
            cv::Size test_size = cv::getTextSize(test_line, font_face, font_scale, thickness, nullptr);
            if (test_size.width <= max_width) {
                current_line = test_line;
            } else {
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                }
                current_line = word;
            }
        }
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
    }
    
    return lines;
}

void GesturesRequester::add_text_to_image(cv::Mat& image, 
                                        const std::string& text, 
                                        double y_pos_percent,
                                        cv::Scalar text_color,
                                        cv::Scalar text_bg_color) {
    const int font_face = cv::FONT_HERSHEY_SIMPLEX;
    const double font_scale = 1.0;
    const int thickness = 2;
    const int margin = 10;
    
    std::vector<std::string> lines = split_text(text, image.cols, font_face, font_scale, thickness, margin);
    int y = static_cast<int>(y_pos_percent * image.rows / 100.0);
    
    for (size_t i = 0; i < lines.size(); ++i) {
        cv::Size text_size = cv::getTextSize(lines[i], font_face, font_scale, thickness, nullptr);
        int x = (image.cols - text_size.width) / 2;
        //int baseline;
        cv::Point text_org(x, y + i * 40);
        
        // Draw background
        cv::putText(image, lines[i], text_org, font_face, font_scale, text_bg_color, thickness + 3, cv::LINE_AA);
        // Draw foreground text
        cv::putText(image, lines[i], text_org, font_face, font_scale, text_color, thickness, cv::LINE_AA);
    }
}


std::vector<std::string> GesturesRequester::split_text_freetype(
    cv::Ptr<cv::freetype::FreeType2> ft2,
    const std::string& text,
    int image_width,
    int font_height,
    int thickness,
    int margin) 
{
    std::vector<std::string> lines;
    std::vector<std::string> input_lines;
    size_t start = 0;
    size_t end = text.find('\n');

    while (end != std::string::npos) {
        input_lines.push_back(text.substr(start, end - start));
        start = end + 1;
        end = text.find('\n', start);
    }
    input_lines.push_back(text.substr(start));

    int max_width = image_width - 2 * margin;

    for (const auto& line : input_lines) {
        cv::Size text_size = ft2->getTextSize(line, font_height, thickness, nullptr);
        if (text_size.width <= max_width) {
            lines.push_back(line);
            continue;
        }

        std::vector<std::string> words;
        size_t word_start = 0;
        size_t space_pos = line.find(' ');
        while (space_pos != std::string::npos) {
            words.push_back(line.substr(word_start, space_pos - word_start));
            word_start = space_pos + 1;
            space_pos = line.find(' ', word_start);
        }
        words.push_back(line.substr(word_start));

        std::string current_line;
        for (const auto& word : words) {
            std::string test_line = current_line.empty() ? word : current_line + " " + word;
            cv::Size test_size = ft2->getTextSize(test_line, font_height, thickness, nullptr);
            if (test_size.width <= max_width) {
                current_line = test_line;
            } else {
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                }
                current_line = word;
            }
        }
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
    }
    return lines;
}


void GesturesRequester::add_text_with_freetype(
    cv::Ptr<cv::freetype::FreeType2> ft2,
    cv::Mat& image,
    const std::string& text,
    double y_pos_percent,
    cv::Scalar text_color,
    cv::Scalar text_bg_color,
    int fontHeight
)
{
    int thickness = -1;
    int margin = 10;

    std::vector<std::string> lines = split_text_freetype(ft2, text, image.cols, fontHeight, thickness, margin);

    int y = static_cast<int>(y_pos_percent * image.rows / 100.0);
    int line_spacing = fontHeight + 6;

    for(size_t i=0; i<lines.size(); ++i) {
        cv::Size text_size = ft2->getTextSize(lines[i], fontHeight, thickness, nullptr);
        int x = (image.cols - text_size.width) / 2;
        int y_i = y + static_cast<int>(i*line_spacing);

        cv::Point text_org(x, y_i);

        int outline_thickness = 10;
        ft2->putText(image, lines[i], text_org, fontHeight, text_bg_color, outline_thickness, cv::LINE_AA, true);
        ft2->putText(image, lines[i], text_org, fontHeight, text_color, thickness, cv::LINE_AA, true);
    }
}


std::pair<std::string, cv::Mat> GesturesRequester::process_requests() {
    if (start_time_) {
        auto now = time_point_cast<milliseconds>(system_clock::now());
        current_gesture_started_at_ = now.time_since_epoch().count();
        start_time_ = false;
    }
    
    if (current_gesture_request_ && current_gesture_index_ > 0) {
        auto now = time_point_cast<milliseconds>(system_clock::now());
        uint64_t current_time = now.time_since_epoch().count();
        uint64_t elapsed = current_time - current_gesture_started_at_;
        
        if (elapsed >= current_gesture_request_->time) {
            if (current_gesture_request_->start_gesture) {
                reset_not_alive();
            } else {
                move_to_next_gesture(current_time);
            }
        }
    }
    
    if (current_gesture_request_) {
        //std::cout << "!!!!!TRANSLATING:" << current_gesture_request_->gestureId << std::endl;
        std::string translated = translator_->translate("gestures." + current_gesture_request_->gestureId + ".label");
        return {translated, current_gesture_request_->icon};
    }
    return {"No gesture", cv::Mat()};
}

void GesturesRequester::reset_not_alive() {
    current_gesture_index_ = 0;
    current_gesture_request_ = &gestures_to_test_[current_gesture_index_];
    if (report_alive_callback_) {
        report_alive_callback_(false);
    }
}

void GesturesRequester::set_report_alive_callback(std::function<void(bool)> callback) {
    report_alive_callback_ = callback;
}

void GesturesRequester::set_ask_to_take_picture_callback(std::function<void()> callback) {
    ask_to_take_picture_callback_ = callback;
}

void GesturesRequester::set_overwrite_text(const std::string& text, bool failure) {
    overwrite_text_ = text;
    overwrite_text_log_.push_back(text);
    if (failure) {
        process_status_ = GesturesRequesterSystemStatus::FAILURE;
    }
}