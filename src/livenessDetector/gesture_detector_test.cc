#include "gesture_detector.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>

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

struct SimParams {
    std::string label;
    int signal_index;
    std::vector<double> values;
    std::vector<int> wait_ms; // in ms, size = values.size()-1
    bool wait_for_hold; // for range gestures
};

int main() {
    GestureDetector detector;
    detector.set_signal_trigger_callback(on_gesture_detected);

    // List all JSONs to test
    std::vector<std::string> file_list = {
        "gesture_threshold.json",
        "gesture_range.json",
        "gesture_perstep_only.json",
        "gesture_perstep_random.json",
        "gesture_perstep_and_end.json",
        "gesture_end_only.json",
        "gesture_nopictures.json",
        "gesture_range_perstep.json",
        "gesture_range_perstep_random.json",
        "gesture_range_and_end.json"
    };

    std::unordered_map<std::string, SimParams> test_cases = {
        // These cases correspond to the gestures above and should be tailored if you change your JSONs
        {"Threshold Test",       {"Threshold Test", 1, {11, 7, 13}, {200,200}, false}},
        {"Range Hold Test",      {"Range Hold Test", 2, {3.2, 3.5}, {2200}, true}},
        {"Per-Step Only",        {"Per-Step Only", 1, {6.0, 1.5, 9.0}, {150,150}, false}},
        {"Per-Step Random",      {"Per-Step Random", 1, {7.0, 2.0, 12.0}, {200, 200}, false}}, 
        {"Per-Step and End",     {"Per-Step and End", 1, {4.0, 10.0}, {250}, false}},
        {"End Only",             {"End Only", 2, {8.0, 3.0}, {200}, false}},
        {"No Pictures",          {"No Pictures", 2, {2.0, 8.0}, {150}, false}},
        {"Range Per-Step",       {"Range Per-Step", 3, {4.0, 4.2}, {1300}, true}},
        {"Range Per-Step Random",{"Range Per-Step Random", 3, {4.4, 3.9}, {1300}, true}},
        {"Range and End",        {"Range and End", 3, {4.2, 4.3}, {1700}, true}}
    };

    // Load all gestures and set photo callbacks for each
    for (const std::string& gesture_file : file_list) {
        std::cout << "\nLoading gesture from: " << gesture_file << "\n";
        auto result = detector.add_gesture_from_file(gesture_file);
        if (!result.success) {
            std::cerr << "Failed to load gesture from file: " << gesture_file << "\n";
        } else {
            std::cout << "  Loaded gesture label: " << result.label << "\n";
            Gesture* gesture = detector.get_gesture_by_label(result.label);
            if (gesture) {
                gesture->set_picture_callback(on_picture_taken);
            }
        }
    }

    // Simulate signals for each loaded gesture
    for (const auto& [label, params] : test_cases) {
        std::cout << "\n=== [" << label << "] Simulating signals ===\n";
        detector.reset_by_label(params.label);
        detector.start_by_label(params.label);

        for (size_t i = 0; i < params.values.size(); ++i) {
            detector.process_signal(params.values[i], params.signal_index);
            // For range gestures, after the first signal we want to "hold" in range for long enough
            if (params.wait_for_hold && i+1 == params.values.size() && params.wait_ms.size()>0) {
                // Stay in range for wait_ms[0] ms to trigger the range completion
                std::this_thread::sleep_for(std::chrono::milliseconds(params.wait_ms[0]));
                detector.process_signal(params.values[i], params.signal_index);
            }
            if (i < params.wait_ms.size() && !(params.wait_for_hold && i+1 == params.values.size())) {
                std::this_thread::sleep_for(std::chrono::milliseconds(params.wait_ms[i]));
            }
        }
    }

    std::cout << "\nAll tests completed.\n";
    return 0;
}