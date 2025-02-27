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
    if (working_) {
        //std::cout << "Gesture update! value: " << value;
        //// Print index if it has a value
        //if (index.has_value()) {
        //    std::cout << ", index: " << *index;
        //} else {
        //    std::cout << ", index: n/a";
        //}
        //// Print signal_key if it has a value
        //if (signal_key.has_value()) {
        //    std::cout << ", signal_key: " << *signal_key;
        //} else {
        //    std::cout << ", signal_key: n/a";
        //}
        //std::cout << std::endl;
        
        if ((index.has_value() && signal_index_.has_value() && (*index == *signal_index_)) ||
            (signal_key.has_value() && signal_key_.has_value() && (*signal_key == *signal_key_))) {
            if (check_(value)) {
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
    }
    return false;
}

void Gesture::stop() {
    working_ = false;
}

void Gesture::reset() {
    current_index_ = 0;
    start_time_ = std::chrono::system_clock::now();
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

bool Gesture::check_(double value) const {
    if (current_index_ >= sequence_.size()) {
        return false;
    }

    const Step& current_step = sequence_[current_index_];
    //std::cout << "Current index: " << current_index_ << ", Move to next step type:";
    
    switch (current_step.move_to_next_type) {
        case Step::MoveType::Higher:
            //std::cout << "Higher, value: " << value << " , current_step value:" << current_step.value << std::endl;
            return value > current_step.value;
        case Step::MoveType::Lower:
            //std::cout << "Lower, value: " << value << " , current_step value:" << current_step.value << std::endl;
            return value < current_step.value;
        default:
            return false;
    }
}

bool Gesture::check_reset_(double value) const {
    if (current_index_ >= sequence_.size()) {
        return false;
    }

    const Step& current_step = sequence_[current_index_];
    if (!current_step.reset.has_value()) {
        return false;
    }

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