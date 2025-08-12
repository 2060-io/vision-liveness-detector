#include "livenessDetector/gesture_detector.h"
#include "livenessDetector/gestures_requester.h"
#include "livenessDetector/translation_manager.h"
#include "livenessDetector/face_processor.h"
#include "livenessDetector/nlohmann/json.hpp"

#include "livenessDetector/unix_socket_transport.h"
#include "livenessDetector/protocol_handler.h"

#include "livenessDetector/glasses_detector.h"
#include "livenessDetector/face_verification.h"


#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <random>
#include <algorithm>
#include <deque>
#include <cctype>
#include <memory>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::map<std::string, std::string> parse_args(int argc, char** argv) {
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; i += 2) {
        if (std::string(argv[i]).find("--") == 0) {
            if (i + 1 < argc) {
                args[argv[i]] = argv[i + 1];
            }
        }
    }
    return args;
}

std::vector<std::string> split_paths(const std::string& paths, char delim = ':') {
    std::vector<std::string> result;
    size_t start = 0, end = 0;
    while ((end = paths.find(delim, start)) != std::string::npos) {
        if (end > start)
            result.push_back(paths.substr(start, end - start));
        start = end + 1;
    }
    if (!paths.substr(start).empty())
        result.push_back(paths.substr(start));
    return result;
}

int main(int argc, char** argv) {

    auto args = parse_args(argc, argv);

    cv::Mat current_input_image;
    json callback_data_json;
    std::string warning_message = "";

    GlassesMode glasses_mode = GlassesMode::OFF;
    std::string glasses_model_path;
    if (args.find("--glasses_detector") != args.end()) {
        glasses_mode = parse_glasses_mode(args["--glasses_detector"]);
    }
    if (args.find("--glasses_model_path") != args.end()) {
        glasses_model_path = args["--glasses_model_path"];
    }
    else {
        glasses_model_path = "glasses_model.onnx";
    }

    // --face_det_model_path (optional)
    std::string face_det_model_path = "";
    if (args.find("--face_det_model_path") != args.end()) {
        face_det_model_path = args["--face_det_model_path"];
    }

    // --max_faceless_attempts (optional)
    int max_faceless_attempts = 10;
    if (args.find("--max_faceless_attempts") != args.end()) {
        try {
            max_faceless_attempts = std::stoi(args["--max_faceless_attempts"]);
        } catch (...) {
            std::cerr << "[Warning] Invalid value for --max_faceless_attempts, using default (10)\n";
            max_faceless_attempts = 10;
        }
    }

    std::vector<std::string> required_keys = {
        "--model_path", "--gestures_folder_path", "--language", "--socket_path", "--num_gestures", "--font_path"
    };
    bool missing = false;
    for (const auto& key : required_keys) {
        if (args.find(key) == args.end()) {
            std::cerr << "Missing required argument: --" << key << std::endl;
            missing = true;
        }
    }
    if (missing) {
        std::cerr << "Usage: " << argv[0]
            << " --model_path <path>"
            << " --gestures_folder_path <path1>:<path2>"
            << " --language <lang>"
            << " --socket_path <path>"
            << " --num_gestures <int>"
            << " --font_path <path>"
            << " [--locales_paths <path1>:<path2>]"
            << " [--gestures_list <gesture1>:<gesture2>:...]"
            << " [--glasses_detector OFF|WARNING_ONLY|ERROR]"
            << " [--glasses_model_path <path_to_glasses_onnx>]"
            << " [--face_det_model_path <path_to_face_cascade_xml>]"
            << " [--max_faceless_attempts <n>]"
            << "\n";
        return EXIT_FAILURE;
    }

    std::string model_path = args["--model_path"];
    std::string gestures_folder_path = args["--gestures_folder_path"];
    std::vector<std::string> gestures_folders = split_paths(gestures_folder_path);
    std::string language = args["--language"];
    std::string socket_path = args["--socket_path"];
    int num_gestures = std::stoi(args["--num_gestures"]);
    std::string font_path = args["--font_path"];

    std::vector<std::string> locales_paths;
    if (args.find("--locales_paths") != args.end())
        locales_paths = split_paths(args["--locales_paths"]);
    for (const auto& folder : gestures_folders)
        locales_paths.push_back(folder + "/locales");

    std::vector<std::string> gestures_list;
    if (args.find("--gestures_list") != args.end()) {
        gestures_list = split_paths(args["--gestures_list"]);
    }

    // ------------- FaceCascadeDetector creation logic --------------
    std::unique_ptr<FaceCascadeDetector> face_detector;
    if (!face_det_model_path.empty()) {
        face_detector = std::make_unique<FaceCascadeDetector>(face_det_model_path);
        if (!face_detector->is_loaded()) {
            std::cerr << "Failed to load Haar cascade from " << face_det_model_path << std::endl;
            face_detector = nullptr;
            // No return EXIT_FAILURE for now
        }
    } else {
        face_detector = nullptr;
    }

    std::unordered_map<std::string, std::string> gesture_name2path;
    for (const auto& folder : gestures_folders) {
        try {
            for (const auto& entry : fs::directory_iterator(folder)) {
                if (entry.path().extension() == ".json") {
                    auto stem = entry.path().stem().string();
                    if (!gesture_name2path.count(stem)) {
                        gesture_name2path[stem] = entry.path().string();
                    }
                }
            }
        }
        catch (fs::filesystem_error& e) {
            std::cerr << "Error accessing the gestures folder: " << folder << ": " << e.what() << "\n";
        }
    }

    std::vector<std::string> gestureFiles;
    std::random_device rd;
    std::mt19937 g(rd());

    if (gestures_list.empty()) {
        for (const auto& kv : gesture_name2path)
            gestureFiles.push_back(kv.second);

        if (num_gestures > static_cast<int>(gestureFiles.size())) {
            std::cerr << "Requested number of gestures exceeds available gestures. Exiting application.\n";
            return EXIT_FAILURE;
        }
        std::shuffle(gestureFiles.begin(), gestureFiles.end(), g);
        gestureFiles.resize(num_gestures);
    }
    else {
        std::vector<std::string> explicitListPaths;
        for (const auto& name : gestures_list) {
            auto it = gesture_name2path.find(name);
            if (it == gesture_name2path.end()) {
                std::cerr << "Gesture '" << name << "' not found in folders. Exiting.\n";
                return EXIT_FAILURE;
            }
            explicitListPaths.push_back(it->second);
        }

        if (static_cast<int>(explicitListPaths.size()) == num_gestures) {
            gestureFiles = explicitListPaths;
        }
        else if (static_cast<int>(explicitListPaths.size()) > num_gestures) {
            std::shuffle(explicitListPaths.begin(), explicitListPaths.end(), g);
            explicitListPaths.resize(num_gestures);
            gestureFiles = explicitListPaths;
        }
        else {
            std::cerr << "gestures_list contains fewer gestures (" << explicitListPaths.size()
                << ") than num_gestures (" << num_gestures << "). Exiting.\n";
            return EXIT_FAILURE;
        }
    }

    if (gestureFiles.empty()) {
        std::cerr << "No gesture JSON files found in the specified folder(s). Exiting application.\n";
        return EXIT_FAILURE;
    }

    GestureDetector detector;
    std::vector<GestureDetector::AddResult> loadedGestures;
    for (const auto& file : gestureFiles) {
        auto result = detector.add_gesture_from_file(file);
        if (!result.success) {
            std::cerr << "Failed to load gesture from file: " << file << "\n";
        }
        else {
            std::cout << "Loaded gesture: " << result.label
                << " (ID: " << result.gestureId << ")\n";
            loadedGestures.push_back(result);
        }
    }
    if (loadedGestures.empty()) {
        std::cerr << "No gestures loaded. Exiting application.\n";
        return EXIT_FAILURE;
    }

    TranslationManager translator(language, locales_paths);

    std::unique_ptr<GlassesDetectorManager> glasses_detector;
    if (glasses_mode != GlassesMode::OFF) {
        glasses_detector = std::make_unique<GlassesDetectorManager>(glasses_model_path, 30);
        if (!glasses_detector->is_loaded()) {
            std::cerr << "Failed to load glasses model from " << glasses_model_path << "\n";
            return EXIT_FAILURE;
        }
    }

    GesturesRequester requester(static_cast<int>(loadedGestures.size()),
        &detector,
        &translator,
        font_path,
        GesturesRequester::DebugLevel::INFO);
    requester.set_gestures_list(loadedGestures);

    // -- This callback ONLY sets the takeAPicture flag! --
    requester.set_ask_to_take_picture_callback([&callback_data_json]() {
        callback_data_json["takeAPicture"] = true;
        std::cout << "[Callback] takeAPicture event set in callback_data_json.\n";
    });

    requester.set_report_alive_callback([&callback_data_json](bool alive) {
        std::cout << "[Callback] GesturesRequester is " << (alive ? "alive" : "not alive") << ".\n";
        callback_data_json["reportAlive"] = alive;
    });

    FaceProcessor processor(model_path);
    processor.SetDoProcessImage(true);

    processor.SetCallback([&detector, &warning_message, &translator, &glasses_detector, glasses_mode, &processor]
        (const std::map<std::string, float>& blendshapes,
            const std::map<std::string, float>& transformationValues) {

            std::unordered_map<std::string, double> convertedBlendshapes;
            for (const auto& pair : blendshapes)
                convertedBlendshapes[pair.first] = static_cast<double>(pair.second);
            detector.process_signals(convertedBlendshapes);

            bool detected_glasses = false;
            if (glasses_detector && (glasses_mode == GlassesMode::WARNING_ONLY || glasses_mode == GlassesMode::ERROR)) {
                const cv::Mat& face_img = processor.GetLastInputImage();
                if (!face_img.empty())
                    detected_glasses = glasses_detector->detect_and_update(face_img);
            }
            if (glasses_mode == GlassesMode::WARNING_ONLY && detected_glasses) {
                warning_message = "Remove the glasses";
            }
            else if (glasses_mode == GlassesMode::ERROR && detected_glasses) {
                warning_message = "Detected glasses. Aborting liveness detection.";
            }
            else {
                warning_message = verify_correct_face(transformationValues, &translator, detected_glasses);
            }
        });

    ProtocolHandler* handler_ptr = nullptr; // will point to actual handler

    auto imageProcessingCallback = [&requester, &processor, &callback_data_json, &current_input_image, &warning_message, &handler_ptr, &face_detector, max_faceless_attempts]
    (const cv::Mat& inputImage) -> std::pair<cv::Mat, std::string>
    {
        current_input_image = inputImage.clone();
        processor.ProcessImage(inputImage);
        std::unordered_map<std::string, double> npoints;
        cv::Mat processedImage = requester.process_image(inputImage, 0, npoints, warning_message);
        std::string callback_data = callback_data_json.empty() ? "" : callback_data_json.dump();

        // --- State for picture attempts ---
        static int no_face_frame_count = 0;

        if (callback_data_json.contains("takeAPicture") && callback_data_json["takeAPicture"] && handler_ptr != nullptr) {
            if (!current_input_image.empty()) {
                cv::Mat img_to_send = current_input_image;
                if (img_to_send.type() != CV_8UC3) {
                    std::cerr << "[CombinedMsg] Image not CV_8UC3 (was type " << img_to_send.type() << "), converting...\n";
                    if (img_to_send.channels() == 1) {
                        cv::cvtColor(img_to_send, img_to_send, cv::COLOR_GRAY2BGR);
                    }
                }
                if (!img_to_send.isContinuous()) img_to_send = img_to_send.clone();

                size_t expected_size = img_to_send.rows * img_to_send.cols * 3;
                size_t actual_size = img_to_send.total() * img_to_send.elemSize();

                if (actual_size == expected_size) {
                    if (face_detector && face_detector->is_loaded()) {
                        std::vector<cv::Rect> faces;
                        bool found = face_detector->detect(img_to_send, &faces);

                        if (found) {
                            std::string json_str = R"({"takeAPicture":true})";
                            std::vector<std::pair<uint32_t, cv::Mat>> images = { {1, img_to_send} };
                            handler_ptr->send_combined(images, json_str);
                            std::cout << "[CombinedMsg] Sent take picture combined message.\n";
                            callback_data_json.erase("takeAPicture");
                            no_face_frame_count = 0;
                        } else {
                            ++no_face_frame_count;
                            if (no_face_frame_count < max_faceless_attempts) {
                                // Prompt for another picture (flag stays true)
                                callback_data_json["takeAPicture"] = true;
                            } else {
                                // Give up and show warning
                                warning_message = "No face detected after multiple attempts.";
                                callback_data_json.erase("takeAPicture");
                                no_face_frame_count = 0;
                            }
                        }
                    } else {
                        // No face detector: always send image!
                        std::string json_str = R"({"takeAPicture":true})";
                        std::vector<std::pair<uint32_t, cv::Mat>> images = { {1, img_to_send} };
                        handler_ptr->send_combined(images, json_str);
                        std::cout << "[CombinedMsg] Sent take picture combined message (no face detection).\n";
                        callback_data_json.erase("takeAPicture"); // turn off so only one image is sent
                        no_face_frame_count = 0;
                    }
                } else {
                    std::cerr << "[CombinedMsg] Image size mismatch! Not sending.\n";
                    // Optionally: still increment counter for timeout here if you want
                }
            }
        }

        // Temporarily save the value of takeAPicture, clear, then restore (if present)
        if (callback_data_json.contains("takeAPicture")) {
            auto takePic = callback_data_json["takeAPicture"];
            callback_data_json.clear();
            callback_data_json["takeAPicture"] = takePic;
        } else {
            callback_data_json.clear();
        }
        return { processedImage, callback_data };
    };

    auto dataProcessingCallback = [&warning_message, &requester](const std::string& json_str) -> std::string {
        try {
            auto j = nlohmann::json::parse(json_str);
            if (j.contains("action") && j["action"] == "set" &&
                j.contains("variable") && j["variable"].is_string() &&
                j.contains("value") && j["value"].is_string())
            {
                const std::string& variable = j["variable"];
                const std::string& value = j["value"];
                if (variable == "warning_message") {
                    warning_message = value;
                    std::cout << "[Config] Set warning_message to: " << warning_message << std::endl;
                }
                else if (variable == "overwrite_text") {
                    requester.set_overwrite_text(value);
                    std::cout << "[Config] Set overwrite_text to: " << value << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Config] JSON parse error: " << e.what() << std::endl;
        }
        return {};
    };

    UnixSocketTransport listener(socket_path);
    if (!listener.open_server()) {
        std::cerr << "Failed to open Unix socket at " << socket_path << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server listening at socket: " << socket_path << ". Waiting for connection from Python client...\n";

    while (true) {
        auto client = listener.accept_client();
        if (!client) {
            std::cerr << "Accept failed, continuing ...\n";
            continue;
        }
        std::cout << "Client connected ...\n";

        ProtocolHandler handler(
            client.get(),
            imageProcessingCallback,
            dataProcessingCallback
        );
        handler_ptr = &handler; // So lambda can send combined

        std::string why_exit;
        while (handler.handle_one_message(why_exit)) {}
        std::cerr << "Session ended: " << why_exit << "\n";
        handler_ptr = nullptr;
    }
    return 0;
}