#pragma once

#include "gesture.h"
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include "nlohmann/json.hpp"

/**
 * @brief Manages multiple gesture objects, loading from files, and signals processing.
 */
class GestureDetector {
public:
    struct AddResult {
        bool success;
        std::string gestureId;
        std::string label;
        std::string icon_path;
        double total_recommended_max_time;
        bool take_picture_at_the_end;
    };

    GestureDetector();
    ~GestureDetector();

    void cleanup();
    void add_gesture(std::unique_ptr<Gesture> gesture);
    AddResult add_gesture_from_file(const std::string& file_path);

    void process_signal(double value, int signal_index);
    void process_signals(const std::unordered_map<std::string, double>& signals);

    void set_signal_trigger_callback(std::function<void(const std::string&)> callback);

    bool reset_all();
    bool reset_by_index(size_t index);
    bool reset_by_label(const std::string& label);

    bool start_all();
    bool start_by_index(size_t index);
    bool start_by_label(const std::string& label);

    bool stop_all();
    bool stop_by_index(size_t index);
    bool stop_by_label(const std::string& label);

    const std::vector<std::unique_ptr<Gesture>>& get_gestures() const;
    Gesture* get_gesture_by_label(const std::string& label) const;

private:
    std::vector<std::unique_ptr<Gesture>> gestures_;
    std::function<void(const std::string&)> signal_trigger_callback_;

    void signal_trigger(Gesture* gesture);
    static std::vector<Gesture::Step> parse_instructions(const nlohmann::json& instructions);
};