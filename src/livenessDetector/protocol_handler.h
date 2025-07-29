#pragma once

#include "transport.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <functional>

using ImageProcessingCallback = std::function<std::pair<cv::Mat, std::string>(const cv::Mat&)>;
using DataProcessingCallback  = std::function<std::string(const std::string&)>;

// This handler is agnostic to the transport implementation!
class ProtocolHandler {
public:
    ProtocolHandler(Transport* transport,
                    ImageProcessingCallback img_callback,
                    DataProcessingCallback data_callback);

    // Serve a client session (handle requests over a connection)
    void serve();

private:
    Transport* m_transport;
    ImageProcessingCallback m_img_cb;
    DataProcessingCallback  m_data_cb;

    bool handle_one_message();

    // Helpers for reading/writing exact bytes
    bool read_exact(void* buf, size_t size);
    bool write_exact(const void* buf, size_t size);

    bool send_image(const cv::Mat& img);
    bool send_json(const std::string& json);

    ProtocolHandler(const ProtocolHandler&) = delete;
    ProtocolHandler& operator=(const ProtocolHandler&) = delete;
};