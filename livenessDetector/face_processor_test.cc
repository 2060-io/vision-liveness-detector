#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <sstream>
#include <chrono>
#include "face_processor.h"

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <model_path> <video1_path> <video2_path> [blendshape_name]\n";
        return EXIT_FAILURE;
    }

    const std::string model_path = argv[1];
    const std::string video1_path = argv[2];
    const std::string video2_path = argv[3];
    const std::string target_blendshape = (argc == 5) ? argv[4] : "";

    // Open output files
    std::ofstream video1_output_file("video1_output.txt");
    if (!video1_output_file.is_open()) {
        std::cerr << "Failed to open video1_output.txt for writing.\n";
        return EXIT_FAILURE;
    }

    std::ofstream video2_output_file("video2_output.txt");
    if (!video2_output_file.is_open()) {
        std::cerr << "Failed to open video2_output.txt for writing.\n";
        return EXIT_FAILURE;
    }

    auto printResults = [&target_blendshape](const std::string& label,
                                             const std::map<std::string, float>& blendshapes,
                                             const std::map<std::string, float>& transformationValues,
                                             std::ofstream& output_file) {
        std::ostringstream oss;
        oss << label << " blendshapes: ";
        if (!target_blendshape.empty()) {
            // Print only the specified blendshape if provided
            auto it = blendshapes.find(target_blendshape);
            if (it != blendshapes.end()) {
                oss << it->first << ": " << it->second;
            } else {
                oss << "(blendshape not found)";
            }
        } else {
            // Print all blendshapes and transformation values
            for (const auto& [name, score] : blendshapes) {
                oss << name << ": " << score << ", ";
            }
            oss << "\nTransformation Values: ";
            for (const auto& [name, value] : transformationValues) {
                oss << name << ": " << value << ", ";
            }
        }
        oss << std::endl;

        // Print to console
        std::cout << oss.str();

        // Write to file
        output_file << oss.str();
    };

    try {
        // Create two FaceProcessor instances
        FaceProcessor processor1(model_path);
        processor1.SetDoProcessImage(true);
        processor1.SetCallback([&](const std::map<std::string, float>& blendshapes,
                                   const std::map<std::string, float>& transformationValues) {
            printResults("Video 1", blendshapes, transformationValues, video1_output_file);
        });

        FaceProcessor processor2(model_path);
        processor2.SetDoProcessImage(true);
        processor2.SetCallback([&](const std::map<std::string, float>& blendshapes,
                                   const std::map<std::string, float>& transformationValues) {
            printResults("Video 2", blendshapes, transformationValues, video2_output_file);
        });

        // Open video files
        cv::VideoCapture cap1(video1_path);
        if (!cap1.isOpened()) {
            std::cerr << "Failed to open video: " << video1_path << std::endl;
            return EXIT_FAILURE;
        }

        cv::VideoCapture cap2(video2_path);
        if (!cap2.isOpened()) {
            std::cerr << "Failed to open video: " << video2_path << std::endl;
            return EXIT_FAILURE;
        }

        // Process frames from both videos
        cv::Mat frame1, frame2;
        while (true) {
            bool has_frame1 = cap1.read(frame1);
            bool has_frame2 = cap2.read(frame2);

            if (!has_frame1 && !has_frame2) {
                break;  // Both videos are ended
            }

            if (has_frame1) {
                processor1.ProcessImage(frame1);
            }
            if (has_frame2) {
                processor2.ProcessImage(frame2);
            }

            // Wait some time to simulate frame processing time, if needed
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Close the output files
    video1_output_file.close();
    video2_output_file.close();

    return EXIT_SUCCESS;
}