#pragma once

#include <vector>
#include <optional>
#include <string>
#include <chrono>
#include <functional>
#include <random>

class Gesture {
public:
    struct Step {
        enum class StepType { Threshold, Range };
        StepType step_type = StepType::Threshold;  // Default to Threshold for backwards compatibility

        // For Threshold step:
        enum class MoveType { Higher, Lower };
        MoveType move_to_next_type;
        double value = 0.0;

        // For Range step:
        double min_value = 0.0;
        double max_value = 0.0;
        int min_duration_ms = 0;

        struct ResetCondition {
            enum class Type { Lower, Higher, TimeoutAfterMs };
            Type type = Type::TimeoutAfterMs;
            double value = 0.0; // For TimeoutAfterMs, this is ms. For Higher/Lower, it's a threshold.
        };
        std::optional<ResetCondition> reset;

        // For range holding, we need to track the entry time for this step
        mutable std::optional<std::chrono::system_clock::time_point> entered_range_time;

        // Per-step: Should we take a picture at the end of this step?
        bool take_picture_at_the_end = false;
    };

    Gesture(std::string gestureId,
            std::string label,
            double total_recommended_max_time,
            bool take_picture_at_the_end,
            std::vector<Step> sequence,
            std::optional<int> signal_index = std::nullopt,
            std::optional<std::string> signal_key = std::nullopt,
            bool randomize_step_picture = false);

    // 'picture_callback_' will be called with the step index where a picture is taken (per step)
    void set_picture_callback(std::function<void(int)> cb);

    bool update(double value, std::optional<int> index = std::nullopt, std::optional<std::string> signal_key = std::nullopt);
    void stop();
    void reset();
    std::string get_label() const;
    double get_total_recommended_max_time() const;
    bool get_working() const;
    std::string get_gesture_id() const;
    std::optional<int> get_signal_index() const;
    std::optional<std::string> get_signal_key() const;
    bool get_take_picture_at_the_end() const;
    const std::vector<Step>& get_sequence() const;
    size_t get_current_index() const;
    std::chrono::system_clock::time_point get_start_time() const;
    void start();

private:
    bool working_;
    std::string gestureId_;
    std::string label_;
    std::optional<int> signal_index_;
    std::optional<std::string> signal_key_;
    double total_recommended_max_time_;
    bool take_picture_at_the_end_;
    std::vector<Step> sequence_;
    size_t current_index_;
    std::chrono::system_clock::time_point start_time_;

    // NEW: randomize per-step picture & chosen step for picture
    bool randomize_step_picture_;
    std::optional<size_t> chosen_picture_step_;
    void select_random_picture_step();

    std::function<void(int)> picture_callback_;

    bool check_step_threshold_(double value, const Step& step) const;
    bool check_step_range_(double value, Step& step) const;
    bool check_reset_(double value) const;
};