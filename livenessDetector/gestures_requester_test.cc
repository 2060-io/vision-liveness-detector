#include "gestures_requester.h"
#include "gesture_detector.h"
#include "translation_manager.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>   // For std::this_thread::sleep_for
#include <chrono>   // For std::chrono::milliseconds
#include <filesystem> // For filesystem operations
#include <fstream>  // For file output

// Callback functions to test the GesturesRequester callbacks
void report_alive(bool is_alive) {
    std::cout << "Report Alive called: " << (is_alive ? "Yes" : "No") << std::endl;
}

void ask_to_take_picture() {
    std::cout << "Ask to Take Picture called" << std::endl;
}

int main() {
    // Set up a test locale directory and default JSON files
    const std::string locales_dir = "locales";
    std::filesystem::create_directory(locales_dir);

    {
        // Create a demo JSON file for "en.json"
        std::ofstream out_en(locales_dir + "/en.json");
        out_en << R"({
            "greeting": {
                "hello": "Hello, World!",
                "hi": "Hi there!"
            }
        })";
        out_en.close();
    }

    {
        // Create a demo JSON file for "default.json"
        std::ofstream out_default(locales_dir + "/default.json");
        out_default << R"({
            "greeting": {
                "hello": "Hello!"
            }
        })";
        out_default.close();
    }

    // Initialize mock gesture detector and translation manager
    TranslationManager translation_manager("en", locales_dir);
    GestureDetector gesture_detector;

    // Create an instance of GesturesRequester with mock components
    GesturesRequester requester(3, &gesture_detector, &translation_manager, GesturesRequester::DebugLevel::INFO);

    // Set callback functions
    requester.set_report_alive_callback(report_alive);
    requester.set_ask_to_take_picture_callback(ask_to_take_picture);

    // Create gestures with steps and add them to the gesture detector
    std::vector<Gesture::Step> steps = {
        {Gesture::Step::MoveType::Higher, 10.0, std::nullopt},
        {Gesture::Step::MoveType::Lower, 8.0, std::nullopt}
    };

    auto gesture1 = std::make_unique<Gesture>("gesture1", "First Gesture", 5000, false, steps, 1);
    auto gesture2 = std::make_unique<Gesture>("gesture2", "Second Gesture", 5000, false, steps, 2);

    gesture_detector.add_gesture(std::move(gesture1));
    gesture_detector.add_gesture(std::move(gesture2));

    // Add gesture results to GesturesRequester
    GestureDetector::AddResult result1 {true, "gesture1", "Gesture One", "icon_path_1", 5000, false};
    GestureDetector::AddResult result2 {true, "gesture2", "Gesture Two", "icon_path_2", 5000, false};

    requester.set_gestures_list({result1, result2});

    // Process an image with no gestures being detected
    cv::Mat img = cv::Mat::zeros(480, 640, CV_8UC3);
    cv::Mat result_img = requester.process_image(img);
    assert(!result_img.empty());
    std::cout << "Image processed with no gestures detected" << std::endl;

    // Simulate gesture detection
    //gesture_detector.set_signal_trigger_callback([&requester](const std::string& label) {
    gesture_detector.set_signal_trigger_callback([](const std::string& label) {
        std::cout << "Simulating gesture detection for: " << label << "\n";
        // Access to private method was being done directly, which is incorrect
        // Typically, the gesture detection simulation would be managed through the process_signal function or similar
    });

    // Trigger signal processing to test gesture detection
    gesture_detector.process_signal(11, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    gesture_detector.process_signal(11, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "Test completed\n";
    return 0;
}