#include "gesture.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // TEST 1: All-Threshold sequence (original style)
    std::vector<Gesture::Step> sequence = {
        // Threshold: value must go above 10.0 to proceed
        {Gesture::Step::StepType::Threshold, Gesture::Step::MoveType::Higher, 10.0, 0, 0, 0, {{Gesture::Step::ResetCondition::Type::Lower, 5.0}}},
        // Threshold: value must go below 8.0
        {Gesture::Step::StepType::Threshold, Gesture::Step::MoveType::Lower, 8.0, 0, 0, 0, {{Gesture::Step::ResetCondition::Type::TimeoutAfterMs, 5000}}},
        // Threshold: value must go above 12.0
        {Gesture::Step::StepType::Threshold, Gesture::Step::MoveType::Higher, 12.0, 0, 0, 0, std::nullopt}
    };

    int signal_index = 1;
    Gesture gesture("Test gesture", "TestGesture", 30.0, false, sequence, signal_index);

    gesture.start();

    std::cout << "Test: Threshold-only gesture sequence\n";
    std::cout << "Updating value with 11 (should move to next index): " 
              << (gesture.update(11, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Updating value with 9 (should NOT move): " 
              << (gesture.update(9, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Updating value with 7 (should move to next index): " 
              << (gesture.update(7, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Updating value with 13 (should detect gesture now): " 
              << (gesture.update(13, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // Reset and test timeout
    std::cout << "\nTimeout reset test...\n";
    gesture.start();
    std::this_thread::sleep_for(std::chrono::seconds(6));  // Sleep over 5s timeout

    std::cout << "Updating value with 11 after timeout (should reset): "
              << (gesture.update(11, signal_index) ? "Gesture detected!" : "No detection") << "\n";


    // TEST 2: Range-hold gesture
    std::vector<Gesture::Step> range_sequence = {
        // Range: Must stay in [2.0, 4.0] for at least 2000 ms
        {
            Gesture::Step::StepType::Range,
            Gesture::Step::MoveType::Higher,  // unused for range
            0, // used only for threshold
            2.0, 4.0, 2000, // min, max, ms
            {{Gesture::Step::ResetCondition::Type::TimeoutAfterMs, 5000}}
        }
    };

    Gesture hold_gesture("Hold in range", "HoldRange", 10.0, false, range_sequence, signal_index);

    hold_gesture.start();
    std::cout << "\nTest: Range/hold gesture\n";
    std::cout << "Entering range value 3.0 (should not detect yet): "
              << (hold_gesture.update(3.0, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(2100)); // Wait stay time

    // Must still be in range for detection
    std::cout << "Still in range after 2.1s (should detect now): "
              << (hold_gesture.update(3.1, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // Leave range (should reset hold time)
    hold_gesture.start();
    std::cout << "Enter range, but leave early, then return (should not detect):\n";
    std::cout << "Value 3.5: " << (hold_gesture.update(3.5, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Leave range with 4.5 (should reset internal timer): " << (hold_gesture.update(4.5, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    std::cout << "Return to range 3.8 (should not detect, timer restarted): " << (hold_gesture.update(3.8, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Still in range after 1s (should not detect): " << (hold_gesture.update(3.9, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    std::cout << "After 2.2s in range (should detect): " << (hold_gesture.update(2.5, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    std::cout << "Test complete.\n";

    return 0;
}