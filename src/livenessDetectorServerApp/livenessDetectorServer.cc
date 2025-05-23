#include "livenessDetector/gesture_detector.h"
#include "livenessDetector/gestures_requester.h"
#include "livenessDetector/translation_manager.h"
#include "livenessDetector/unix_socket_server.h"
#include "livenessDetector/face_processor.h"
#include "livenessDetector/nlohmann/json.hpp"

#include <opencv2/opencv.hpp> 
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <random>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0] << " <model_path> <gestures_folder_path> <language> <socket_path> <num_gestures> <font_path>\n";
        return EXIT_FAILURE;
    }

    const std::string model_path = argv[1];
    const std::string gestures_folder_path = argv[2];
    const std::string language = argv[3];
    const std::string socket_path = argv[4];
    int num_gestures = std::stoi(argv[5]);
    const std::string font_path = argv[6];

    std::cout << "Starting Liveness Detector Server...\n";

    // Initialize callback_data_ as a JSON object
    json callback_data_json;

    // Load gesture definitions from JSON files.
    std::vector<std::string> gestureFiles;

    try {
        for (const auto& entry : fs::directory_iterator(gestures_folder_path)) {
            if (entry.path().extension() == ".json") {
                gestureFiles.push_back(entry.path().string());
            }
        }
    } catch (fs::filesystem_error& e) {
        std::cerr << "Error accessing the gestures folder: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    if (gestureFiles.empty()) {
        std::cerr << "No gesture JSON files found in the specified folder. Exiting application.\n";
        return EXIT_FAILURE;
    }

    // Ensure we do not attempt to select more gestures than available
    if (num_gestures > static_cast<int>(gestureFiles.size())) {
        std::cerr << "Requested number of gestures exceeds available gestures. Exiting application.\n";
        return EXIT_FAILURE;
    }

    // Randomly shuffle and select the specified number of gestures
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(gestureFiles.begin(), gestureFiles.end(), g);

    // Select only the specified number of gestures
    gestureFiles.resize(num_gestures);

    // Create an instance of GestureDetector
    GestureDetector detector;
    std::vector<GestureDetector::AddResult> loadedGestures;

    for (const auto& file : gestureFiles) {
        auto result = detector.add_gesture_from_file(file);
        if (!result.success) {
            std::cerr << "Failed to load gesture from file: " << file << "\n";
        } else {
            std::cout << "Loaded gesture: " << result.label
                      << " (ID: " << result.gestureId << ")\n";
            loadedGestures.push_back(result);
        }
    }

    if (loadedGestures.empty()) {
        std::cerr << "No gestures loaded. Exiting application.\n";
        return EXIT_FAILURE;
    }

    // Create a TranslationManager using the locales folder inside the gestures_folder_path.
    std::string locales_path = gestures_folder_path + "/locales";
    TranslationManager translator(language, locales_path);

    // Create a GesturesRequester.
    GesturesRequester requester(static_cast<int>(loadedGestures.size()),
                                &detector,
                                &translator,
                                font_path,
                                GesturesRequester::DebugLevel::INFO);

    requester.set_gestures_list(loadedGestures);

    // Capture the local variables by reference in the lambda
    requester.set_ask_to_take_picture_callback([&callback_data_json]() {
        std::cout << "[Callback] Ask to take picture triggered.\n";
        // Add or update the 'takeAPicture' key in the callback_data_json object
        callback_data_json["takeAPicture"] = true;
    });

    requester.set_report_alive_callback([&callback_data_json](bool alive) {
        std::cout << "[Callback] GesturesRequester is " << (alive ? "alive" : "not alive") << ".\n";
        // Add or update the 'reportAlive' key in the callback_data_json object
        callback_data_json["reportAlive"] = alive;
    });

    // Create a FaceProcessor
    FaceProcessor processor(model_path);
    processor.SetDoProcessImage(true);
    processor.SetCallback([&detector](const std::map<std::string, float>& blendshapes,
        const std::map<std::string, float>& transformationValues) {
            // Create an unordered_map to hold the converted blendshapes
            std::unordered_map<std::string, double> convertedBlendshapes;
            // Convert each element type
            for (const auto& pair : blendshapes) {
                convertedBlendshapes[pair.first] = static_cast<double>(pair.second);
            }
            // Call process_signals with the converted map
            detector.process_signals(convertedBlendshapes);
    });

    // Create a UnixSocketServer using the provided socket path.
    auto imageProcessingCallback = [&requester, &processor, &callback_data_json](const cv::Mat& inputImage) -> std::pair<cv::Mat, std::string> {
        processor.ProcessImage(inputImage);

        std::unordered_map<std::string, double> npoints;  // empty points; extend as needed!
        std::string warning_message = "";
        cv::Mat processedImage = requester.process_image(inputImage, 0, npoints, warning_message);

        // Serialize the accumulated JSON object to a string
        std::string callback_data = callback_data_json.empty() ? "" : callback_data_json.dump();
        callback_data_json.clear();

        return {processedImage, callback_data};
    };

    UnixSocketServer socketServer(socket_path, imageProcessingCallback);

    if (!socketServer.start()) {
        std::cerr << "Failed to start UnixSocketServer.\n";
        return EXIT_FAILURE;
    }

    std::cout << "UnixSocketServer started at path: " << socket_path << ". Waiting for connection from Python client...\n";

    socketServer.run();

    return 0;
}