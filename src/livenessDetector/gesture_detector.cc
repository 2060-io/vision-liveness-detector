#include "gesture_detector.h"
#include <fstream>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

GestureDetector::GestureDetector() {
    std::cout << "GestureDetector created.\n";
}

GestureDetector::~GestureDetector() {
    std::cout << "GestureDetector destroyed.\n";
    cleanup();
}

void GestureDetector::cleanup() {
    gestures_.clear();
}

void GestureDetector::add_gesture(std::unique_ptr<Gesture> gesture) {
    gestures_.push_back(std::move(gesture));
}

GestureDetector::AddResult GestureDetector::add_gesture_from_file(const std::string& file_path) {
    AddResult result{false, "", "", "", 0.0, false};
    
    if (!fs::exists(file_path)) {
        std::cerr << "File does not exist: " << file_path << "\n";
        return result;
    }

    try {
        std::ifstream f(file_path);
        json gesture_data = json::parse(f);
        
        // Basic validation
        if (!gesture_data.contains("gestureId") || 
            !gesture_data.contains("label") ||
            !gesture_data.contains("instructions")) {
            std::cerr << "Invalid gesture data: missing required fields.\n";
            return result;
        }

        std::optional<int> signal_index;
        std::optional<std::string> signal_key;
        
        if (gesture_data.contains("signal_index")) {
            signal_index = gesture_data["signal_index"].get<int>();
        }
        if (gesture_data.contains("signal_key")) {
            signal_key = gesture_data["signal_key"].get<std::string>();
        }

        auto gesture = std::make_unique<Gesture>(
            gesture_data["gestureId"].get<std::string>(),
            gesture_data["label"].get<std::string>(),
            gesture_data["total_recommended_max_time"].get<double>(),
            gesture_data["take_picture_at_the_end"].get<bool>(),
            parse_instructions(gesture_data["instructions"]),
            signal_index,
            signal_key
        );
        std::cout << "New gesture with size: " << gesture->get_sequence().size() << std::endl;
        
        result.success = true;
        result.gestureId = gesture->get_gesture_id();
        result.label = gesture->get_label();
        result.total_recommended_max_time = gesture->get_total_recommended_max_time();
        result.take_picture_at_the_end = gesture->get_take_picture_at_the_end();
        
        if (gesture_data.contains("icon_path")) {
            result.icon_path = gesture_data["icon_path"].get<std::string>();
        }

        gestures_.push_back(std::move(gesture));
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing gesture file: " << e.what() << "\n";
    }
    
    return result;
}

void GestureDetector::process_signal(double value, int signal_index) {
    for (auto& gesture : gestures_) {
        if (gesture->get_working()) {
            if (gesture->update(value, signal_index)) {
                signal_trigger(gesture.get());
            }
        }
    }
}

void GestureDetector::process_signals(const std::unordered_map<std::string, double>& signals) {
    for (auto& gesture : gestures_) {
        if (gesture->get_working() && gesture->get_signal_key().has_value()) {
            //std::cout << "Gesture: " << gesture->get_gesture_id() << ", Working: " << gesture->get_working() << ", signal key: " << gesture->get_signal_key().value() << std::endl;
            auto it = signals.find(gesture->get_signal_key().value());
            if (it != signals.end()) {
                //std::cout << "Signal Found!!: " << gesture->get_signal_key().value() << std::endl;
                if (gesture->update(it->second, {}, it->first)) {
                    signal_trigger(gesture.get());
                }
            }
        }
    }
}

void GestureDetector::set_signal_trigger_callback(std::function<void(const std::string&)> callback) {
    signal_trigger_callback_ = callback;
}

bool GestureDetector::reset_all() {
    for (auto& gesture : gestures_) {
        gesture->reset();
    }
    return !gestures_.empty();
}

bool GestureDetector::reset_by_index(size_t index) {
    if (index < gestures_.size()) {
        gestures_[index]->reset();
        return true;
    }
    return false;
}

bool GestureDetector::reset_by_label(const std::string& label) {
    for (auto& gesture : gestures_) {
        if (gesture->get_label() == label) {
            gesture->reset();
            return true;
        }
    }
    return false;
}

bool GestureDetector::start_all() {
    for (auto& gesture : gestures_) {
        gesture->start();
    }
    return !gestures_.empty();
}

bool GestureDetector::start_by_index(size_t index) {
    if (index < gestures_.size()) {
        gestures_[index]->start();
        return true;
    }
    return false;
}

bool GestureDetector::start_by_label(const std::string& label) {
    for (auto& gesture : gestures_) {
        if (gesture->get_label() == label) {
            gesture->start();
            return true;
        }
    }
    return false;
}

bool GestureDetector::stop_all() {
    for (auto& gesture : gestures_) {
        gesture->stop();
    }
    return !gestures_.empty();
}

bool GestureDetector::stop_by_index(size_t index) {
    if (index < gestures_.size()) {
        gestures_[index]->stop();
        return true;
    }
    return false;
}

bool GestureDetector::stop_by_label(const std::string& label) {
    for (auto& gesture : gestures_) {
        if (gesture->get_label() == label) {
            gesture->stop();
            return true;
        }
    }
    return false;
}

const std::vector<std::unique_ptr<Gesture>>& GestureDetector::get_gestures() const {
    return gestures_;
}

Gesture* GestureDetector::get_gesture_by_label(const std::string& label) const {
    for (auto& gesture : gestures_) {
        if (gesture->get_label() == label) {
            return gesture.get();
        }
    }
    return nullptr;
}

void GestureDetector::signal_trigger(Gesture* gesture) {
    if (signal_trigger_callback_) {
        signal_trigger_callback_(gesture->get_label());
    }
}

std::vector<Gesture::Step> GestureDetector::parse_instructions(const nlohmann::json& instructions) {
    std::vector<Gesture::Step> steps;

    for (const auto& item : instructions) {
        Gesture::Step::MoveType move_type = (item["move_to_next_type"] == "higher") ? 
            Gesture::Step::MoveType::Higher : Gesture::Step::MoveType::Lower;

        double value = item["value"];

        std::optional<Gesture::Step::ResetCondition> reset_condition = std::nullopt;

        if (item.contains("reset")) {
            const auto& reset_data = item["reset"];
            Gesture::Step::ResetCondition::Type reset_type;
            //std::cout << "Reset Data Type: " << reset_data["type"] << std::endl;
            if (reset_data["type"] == "lower") {
                reset_type = Gesture::Step::ResetCondition::Type::Lower;
            } else if (reset_data["type"] == "higher") {
                reset_type = Gesture::Step::ResetCondition::Type::Higher;
            } else if (reset_data["type"] == "timeout_after_ms") {
                reset_type = Gesture::Step::ResetCondition::Type::TimeoutAfterMs;
            } else {
                std::cerr << "Unknown reset condition type.\n";
                continue;
            }

            double reset_value = reset_data["value"];
            reset_condition = Gesture::Step::ResetCondition{ reset_type, reset_value };
        }

        steps.push_back(Gesture::Step{ move_type, value, reset_condition });
    }

    return steps;
}