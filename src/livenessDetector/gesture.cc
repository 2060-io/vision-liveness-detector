#include "gesture.h"
#include <chrono>
#include <iostream>

Gesture::Gesture(std::string gestureId,
                 std::string label,
                 double total_recommended_max_time,
                 bool take_picture_at_the_end,
                 std::vector<Step> sequence,
                 std::optional<int> signal_index,
                 std::optional<std::string> signal_key)
    : working_(false),
      gestureId_(gestureId),
      label_(std::move(label)),
      signal_index_(signal_index),
      signal_key_(std::move(signal_key)),
      total_recommended_max_time_(total_recommended_max_time),
      take_picture_at_the_end_(take_picture_at_the_end),
      sequence_(std::move(sequence)),
      current_index_(0),
      start_time_(std::chrono::system_clock::now()) {}

bool Gesture::update(double value, std::optional<int> index, std::optional<std::string> signal_key) {
    if (!working_)
        return false;

    if ((index.has_value() && signal_index_.has_value() && (*index == *signal_index_)) ||
        (signal_key.has_value() && signal_key_.has_value() && (*signal_key == *signal_key_))) {
        if (current_index_ >= sequence_.size())
            return false;

        Step& curr_step = sequence_[current_index_]; // Non-const, because for Range we need mutable

        bool step_complete = false;
        switch (curr_step.step_type) {
            case Step::StepType::Threshold:
                step_complete = check_step_threshold_(value, curr_step);
                break;
            case Step::StepType::Range:
                step_complete = check_step_range_(value, curr_step);
                break;
            default:
                // Unknown type, ignore
                break;
        }
        if (step_complete) {
            curr_step.entered_range_time.reset(); // Reset in case we have range tracking
            if (current_index_ == 0) {
                start_time_ = std::chrono::system_clock::now();
            }
            current_index_++;
            if (current_index_ >= sequence_.size()) {
                return true;
            }
        } else {
            if (check_reset_(value)) {
                reset();
            }
        }
    }
    return false;
}

void Gesture::stop() {
    working_ = false;
}

void Gesture::reset() {
    current_index_ = 0;
    start_time_ = std::chrono::system_clock::now();
    // Reset range timing for all steps
    for (auto& step : sequence_) {
        step.entered_range_time.reset();
    }
}

std::string Gesture::get_label() const {
    return label_;
}

double Gesture::get_total_recommended_max_time() const {
    return total_recommended_max_time_;
}

bool Gesture::get_working() const {
    return working_;
}

std::string Gesture::get_gesture_id() const {
    return gestureId_;
}

std::optional<int> Gesture::get_signal_index() const {
    return signal_index_;
}

std::optional<std::string> Gesture::get_signal_key() const {
    return signal_key_;
}

bool Gesture::get_take_picture_at_the_end() const {
    return take_picture_at_the_end_;
}

const std::vector<Gesture::Step>& Gesture::get_sequence() const {
    return sequence_;
}

size_t Gesture::get_current_index() const {
    return current_index_;
}

std::chrono::system_clock::time_point Gesture::get_start_time() const {
    return start_time_;
}

void Gesture::start() {
    reset();
    working_ = true;
}

bool Gesture::check_step_threshold_(double value, const Step& step) const {
    switch (step.move_to_next_type) {
        case Step::MoveType::Higher:
            return value > step.value;
        case Step::MoveType::Lower:
            return value < step.value;
        default:
            return false;
    }
}

bool Gesture::check_step_range_(double value, Step& step) const {
    using clock = std::chrono::system_clock;
    auto now = clock::now();

    // Check if we are inside the range
    if (value >= step.min_value && value <= step.max_value) {
        // If not already started, set start time
        if (!step.entered_range_time.has_value()) {
            step.entered_range_time = now;
            return false; // Need to stay for duration
        } else {
            // Check duration
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - *step.entered_range_time);
            if (duration.count() >= step.min_duration_ms) {
                step.entered_range_time.reset(); // clear for next run
                return true; // Range held for enough time!
            }
            return false;
        }
    } else {
        // Left the required range: reset timer
        step.entered_range_time.reset();
        return false;
    }
}

bool Gesture::check_reset_(double value) const {
    if (current_index_ >= sequence_.size())
        return false;

    const Step& current_step = sequence_[current_index_];
    if (!current_step.reset.has_value())
        return false;

    const Step::ResetCondition& reset = current_step.reset.value();
    switch (reset.type) {
        case Step::ResetCondition::Type::Lower:
            return value < reset.value;
        case Step::ResetCondition::Type::Higher:
            return value > reset.value;
        case Step::ResetCondition::Type::TimeoutAfterMs: {
            using namespace std::chrono;
            auto now = system_clock::now();
            duration<double, std::milli> elapsed = now - start_time_;
            return elapsed.count() > reset.value;
        }
        default:
            return false;
    }
}