#pragma once

#include <opencv2/opencv.hpp>
#include <functional>
#include <string>
#include <vector>

class UnixSocketServer {
public:
    using ImageProcessingCallback = std::function<std::pair<cv::Mat, std::string>(const cv::Mat&)>;

    UnixSocketServer(const std::string& socketPath, ImageProcessingCallback callback);
    ~UnixSocketServer();

    bool start();
    void run();

private:
    std::string socketPath;
    ImageProcessingCallback processImageCallback;
    int server_fd;

    bool processClient(int client_fd);
};