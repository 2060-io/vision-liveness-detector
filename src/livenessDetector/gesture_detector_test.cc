#include "gesture_detector.h"
#include <iostream>
#include <thread>
#include <chrono>

void on_gesture_detected(const std::string& label) {
    std::cout << "[CALLBACK] Gesture detected: " << label << "\n";
}

void on_picture_taken(int step) {
    if (step == -1) {
        std::cout << "[PICTURE] At gesture END\n";
    } else {
        std::cout << "[PICTURE] At step index " << step << "\n";
    }
}

int main() {
    GestureDetector detector;
    detector.set_signal_trigger_callback(on_gesture_detected);

    std::string file_list[] = {"gesture_threshold.json", "gesture_range.json"};

    for (const std::string& gesture_file : file_list) {
        std::cout << "\nLoading gesture from: " << gesture_file << "\n";
        auto result = detector.add_gesture_from_file(gesture_file);
        if (!result.success) {
            std::cerr << "Failed to load gesture from file: " << gesture_file << "\n";
        } else {
            std::cout << "  Loaded gesture label: " << result.label << "\n";
            // ------ ADD THIS BLOCK ------
            Gesture* gesture = detector.get_gesture_by_label(result.label);
            if (gesture) {
                gesture->set_picture_callback(on_picture_taken);
            }
            // ----------------------------
        }
    }

    // --------------------------------------
    // Test threshold gesture
    // --------------------------------------
    std::cout << "\n[THRESHOLD GESTURE] Simulating signals...\n";
    detector.start_by_label("Threshold Test");

    detector.process_signal(11, 1); // triggers step 0 (should photo)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    detector.process_signal(9, 1);  // not low enough yet for step 1
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    detector.process_signal(7, 1);  // triggers step 1 (should photo)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    detector.process_signal(13, 1); // triggers gesture end

    detector.reset_by_label("Threshold Test");
    detector.start_by_label("Threshold Test");
    std::cout << "[THRESHOLD] Simulate timeout...\n";
    std::this_thread::sleep_for(std::chrono::seconds(6));
    detector.process_signal(11, 1); // should have reset due to timeout

    // --------------------------------------
    // Test range gesture
    // --------------------------------------
    std::cout << "\n[RANGE GESTURE] Simulating signals...\n";
    detector.start_by_label("Range Hold Test");

    detector.process_signal(3, 2); // into range
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    detector.process_signal(3.5, 2); // still in range, not enough yet
    std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // total ~2.1s
    detector.process_signal(3.5, 2); // triggers step or randomizes (should photo: only one pic at step for randomize)

    detector.reset_by_label("Range Hold Test");
    detector.start_by_label("Range Hold Test");
    detector.process_signal(3, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    detector.process_signal(4.5, 2); // out of range resets
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    detector.process_signal(3.1, 2); // back in range -- hold time restarts
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    detector.process_signal(3.05, 2); // still not detected
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    detector.process_signal(2.9, 2); // now enough time in range -- step detected (picture should print if random chooses it)

    std::cout << "Test completed.\n";
    return 0;
}