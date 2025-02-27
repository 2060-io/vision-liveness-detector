#include "gesture_detector.h"
#include <iostream>
#include <thread>
#include <chrono>

// Callback function for when a gesture is detected
void on_gesture_detected(const std::string& label) {
    std::cout << "Gesture detected: " << label << "\n";
}

int main() {
    // Create a GestureDetector instance
    GestureDetector detector;

    // Set the callback for gesture detection
    detector.set_signal_trigger_callback(on_gesture_detected);

    // Load a gesture from a JSON file
    std::string gesture_file = "gesture.json"; // Path to your gesture JSON file
    auto result = detector.add_gesture_from_file(gesture_file);

    if (!result.success) {
        std::cerr << "Failed to load gesture from file: " << gesture_file << "\n";
        return 1;
    }

    std::cout << "Gesture loaded successfully:\n";
    std::cout << "  ID: " << result.gestureId << "\n";
    std::cout << "  Label: " << result.label << "\n";
    std::cout << "  Max Time: " << result.total_recommended_max_time << "\n";
    std::cout << "  Take Picture: " << (result.take_picture_at_the_end ? "Yes" : "No") << "\n";

    // Start the gesture detection
    detector.start_all();

    // Simulate signal updates
    std::cout << "Simulating signal updates...\n";

    // Test case 1: Move to the next step
    detector.process_signal(11, 1); // Should move to the next step
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Test case 2: Do not move to the next step
    detector.process_signal(9, 1); // Should not move to the next step
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Test case 3: Move to the next step
    detector.process_signal(7, 1); // Should move to the next step
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Test case 4: Complete the gesture
    detector.process_signal(13, 1); // Should trigger gesture detection
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Reset and test timeout
    std::cout << "Resetting for timeout test...\n";
    detector.reset_all();
    detector.start_all();

    // Simulate timeout
    std::this_thread::sleep_for(std::chrono::seconds(6)); // Wait for timeout
    detector.process_signal(11, 1); // Should reset due to timeout

    std::cout << "Test completed.\n";
    return 0;
}