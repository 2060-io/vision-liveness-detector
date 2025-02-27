#include "unix_socket_server.h"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    auto processImage = [](const cv::Mat& img) -> std::pair<cv::Mat, std::string> {
        cv::Mat processed_img;
        cv::cvtColor(img, processed_img, cv::COLOR_BGR2GRAY);
        cv::cvtColor(processed_img, processed_img, cv::COLOR_GRAY2BGR);

        std::string info = "Processed image with size: " + std::to_string(processed_img.total()) + " pixels";

        return {processed_img, info};
    };

    UnixSocketServer server("/tmp/mysocket", processImage);

    if (!server.start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }

    server.run();

    return 0;
}