#pragma once

#include <opencv2/opencv.hpp>
#include <functional>
#include <string>
#include <vector>

class UnixSocketServer {
public:
    using ImageProcessingCallback = std::function<std::pair<cv::Mat, std::string>(const cv::Mat&)>;
    using DataProcessingCallback = std::function<std::string(const std::string&)>;

    UnixSocketServer(
        const std::string& socketPath,
        ImageProcessingCallback imgCallback,
        DataProcessingCallback dataCallback
    );
    ~UnixSocketServer();

    bool start();
    void run();

private:
    std::string socketPath;
    ImageProcessingCallback processImageCallback;
    DataProcessingCallback processDataCallback;
    int server_fd;

    bool processClient(int client_fd);
};