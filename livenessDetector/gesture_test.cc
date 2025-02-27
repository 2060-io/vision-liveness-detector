#include "gesture.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // Define a sequence of checks with move_to_next_type and reset conditions
    std::vector<Gesture::Step> sequence = {
        {Gesture::Step::MoveType::Higher, 10.0, {{Gesture::Step::ResetCondition::Type::Lower, 5.0}}},
        {Gesture::Step::MoveType::Lower, 8.0, {{Gesture::Step::ResetCondition::Type::TimeoutAfterMs, 5000}}},
        {Gesture::Step::MoveType::Higher, 12.0, std::nullopt}
    };

    // Instantiate a Gesture object with a signal_index
    int signal_index = 1; // Example signal index
    Gesture gesture("Test gesture", "TestGesture", 30.0, false, sequence, signal_index);

    // Start the gesture detection
    gesture.start();

    // Test sequence of updates
    std::cout << "Starting test sequence...\n";

    // Test case 1: Expect to move to the next state
    std::cout << "Updating value with 11 (should move to the next index): " 
              << (gesture.update(11, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // Test case 2: Expect not to move as value is not lower than 8
    std::cout << "Updating value with 9 (should NOT move to the next index): " 
              << (gesture.update(9, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // Test case 3: Correct sequence leading to detection
    std::cout << "Updating value with 7 (should move to the next index due to correct lower condition): " 
              << (gesture.update(7, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Updating value with 13 (should detect gesture now): " 
              << (gesture.update(13, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // Reset and re-test
    std::cout << "\nResetting for a timeout condition test...\n";
    gesture.start();

    // Simulate timeout
    std::this_thread::sleep_for(std::chrono::seconds(6));  // Sleep to simulate the timeout condition

    // After timeout, update should reset
    std::cout << "Updating value with 11 after timeout (should reset): " 
              << (gesture.update(11, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    std::cout << "Test sequence completed.\n";

    return 0;
}