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
#include <map>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <random>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Function to verify if the detected face satisfies all constraints.
// Returns an empty string if OK, otherwise a warning message.
std::string verify_correct_face(
    const std::map<std::string, float>& face_square_normalized_points,
    TranslationManager* translator,
    bool glasses = false,
    float percentage_min_face_width = 0.1f,
    float percentage_max_face_width = 0.5f,
    float percentage_min_face_height = 0.1f,
    float percentage_max_face_height = 0.7f,
    float percentage_center_allowed_offset = 0.25f
) {
    const std::string wrong_face_width_message   = translator->translate("warning.wrong_face_width_message");
    const std::string wrong_face_height_message  = translator->translate("warning.wrong_face_height_message");
    const std::string wrong_face_center_message  = translator->translate("warning.wrong_face_center_message");
    const std::string face_not_detected_message  = translator->translate("warning.face_not_detected_message");
    const std::string face_with_glasses_message  = translator->translate("warning.face_with_glasses_message");

    // 1. Check if points exist
    const char* required_keys[] = {"Top Square", "Left Square", "Right Square", "Bottom Square"};
    for (const auto& key : required_keys) {
        if (face_square_normalized_points.find(key) == face_square_normalized_points.end()) {
            return face_not_detected_message;
        }
    }

    // 2. Glasses check
    if (glasses) {
        return face_with_glasses_message;
    }

    // 3. Get coords
    float topSquare    = face_square_normalized_points.at("Top Square");
    float leftSquare   = face_square_normalized_points.at("Left Square");
    float rightSquare  = face_square_normalized_points.at("Right Square");
    float bottomSquare = face_square_normalized_points.at("Bottom Square");

    // Quick fail if not valid detection
    if (topSquare < 0 || leftSquare < 0 || rightSquare < 0 || bottomSquare < 0) {
        return face_not_detected_message;
    }

    // 4. Face width/height, center
    float face_width  = rightSquare - leftSquare;
    float face_height = bottomSquare - topSquare;
    float face_center_x = (rightSquare + leftSquare) / 2.0f;
    float face_center_y = (topSquare    + bottomSquare) / 2.0f;

    // 5. Checks
    if (!(percentage_min_face_width <= face_width && face_width <= percentage_max_face_width)) {
        return wrong_face_width_message;
    }
    if (!(percentage_min_face_height <= face_height && face_height <= percentage_max_face_height)) {
        return wrong_face_height_message;
    }
    float center = 0.5f;
    if (!(center - percentage_center_allowed_offset <= face_center_x && face_center_x <= center + percentage_center_allowed_offset) ||
        !(center - percentage_center_allowed_offset <= face_center_y && face_center_y <= center + percentage_center_allowed_offset)) {
        return wrong_face_center_message;
    }

    // OK!
    return "";
}

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

    std::string warning_message = "";

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
    processor.SetCallback([&detector, &warning_message, &translator](const std::map<std::string, float>& blendshapes,
        const std::map<std::string, float>& transformationValues) {
            // Create an unordered_map to hold the converted blendshapes
            std::unordered_map<std::string, double> convertedBlendshapes;
            // Convert each element type
            for (const auto& pair : blendshapes) {
                convertedBlendshapes[pair.first] = static_cast<double>(pair.second);
            }
            // Call process_signals with the converted map
            detector.process_signals(convertedBlendshapes);
            warning_message = verify_correct_face(transformationValues, &translator);
    });

    // Create a UnixSocketServer using the provided socket path.
    auto imageProcessingCallback = [&requester, &processor, &callback_data_json, &warning_message](const cv::Mat& inputImage) -> std::pair<cv::Mat, std::string> {
        processor.ProcessImage(inputImage);

        std::unordered_map<std::string, double> npoints;  // empty points; extend as needed!
        cv::Mat processedImage = requester.process_image(inputImage, 0, npoints, warning_message);

        // Serialize the accumulated JSON object to a string
        std::string callback_data = callback_data_json.empty() ? "" : callback_data_json.dump();
        callback_data_json.clear();

        return {processedImage, callback_data};
    };
 
    auto dataProcessingCallback = [&warning_message, &requester](const std::string& json_str) -> std::string {
        try {
            auto j = nlohmann::json::parse(json_str);
    
            // Check all required fields before proceeding
            if (j.contains("action") && j["action"] == "set" &&
                j.contains("variable") && j["variable"].is_string() &&
                j.contains("value") && j["value"].is_string())
            {
                const std::string& variable = j["variable"];
                const std::string& value = j["value"];
    
                if (variable == "warning_message") {
                    warning_message = value;
                    std::cout << "[Config] Set warning_message to: " << warning_message << std::endl;
                } else if (variable == "overwrite_text") {
                    requester.set_overwrite_text(value);
                    std::cout << "[Config] Set overwrite_text to: " << value << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Config] JSON parse error: " << e.what() << std::endl;
        }
        return {};
    };

    UnixSocketServer socketServer(
        socket_path, 
        imageProcessingCallback,
        dataProcessingCallback
    );

    if (!socketServer.start()) {
        std::cerr << "Failed to start UnixSocketServer.\n";
        return EXIT_FAILURE;
    }

    std::cout << "UnixSocketServer started at path: " << socket_path << ". Waiting for connection from Python client...\n";

    socketServer.run();

    return 0;
}