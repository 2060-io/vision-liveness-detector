#pragma once

#include <vector>
#include <optional>
#include <string>
#include <chrono>

class Gesture {
public:
    struct Step {
        enum class MoveType { Higher, Lower };
        MoveType move_to_next_type;
        double value;

        struct ResetCondition {
            enum class Type { Lower, Higher, TimeoutAfterMs };
            Type type;
            double value;
        };
        std::optional<ResetCondition> reset;
    };

    Gesture(std::string gestureId,
            std::string label,
            double total_recommended_max_time,
            bool take_picture_at_the_end,
            std::vector<Step> sequence,
            std::optional<int> signal_index = std::nullopt,
            std::optional<std::string> signal_key = std::nullopt);

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

    bool check_(double value) const;
    bool check_reset_(double value) const;
};