#pragma once

#include "transport.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <limits>
#include <iostream>

using ImageProcessingCallback = std::function<std::pair<cv::Mat, std::string>(const cv::Mat&)>;
using DataProcessingCallback  = std::function<std::string(const std::string&)>;

// Message type ID in protocol
enum class ProtocolMessageType : uint8_t {
    Image = 0x01,
    Json  = 0x02,
};

// Diagnostic: how big of a message can be processed
constexpr size_t MAX_FRAME_SIZE = 16 * 1024 * 1024; // 16 MB max for uncompressed frame
constexpr size_t MAX_JSON_SIZE  = 1024 * 1024;      // 1 MB for JSON messages

// This handler is agnostic to the transport implementation!
// NOTE: The transport pointer is NOT owned by ProtocolHandler.
// The caller must ensure the transport is alive for the life of this handler.
class ProtocolHandler {
public:
    ProtocolHandler(Transport* transport,
                    ImageProcessingCallback img_callback,
                    DataProcessingCallback data_callback);

    // Serve a client session (handle requests over a connection)
    // Returns false on fatal error/connection closed, true on graceful exit.
    bool serve();

private:
    Transport* m_transport;
    ImageProcessingCallback m_img_cb;
    DataProcessingCallback  m_data_cb;

    // Helpers for reading/writing exact bytes with size limit
    bool read_exact(void* buf, size_t size);
    bool write_exact(const void* buf, size_t size);

    bool send_image(const cv::Mat& img);
    bool send_json(const std::string& json);

    // Handle one full incoming message; returns false on fatal I/O or protocol error.
    bool handle_one_message(std::string& error_out);

    ProtocolHandler(const ProtocolHandler&) = delete;
    ProtocolHandler& operator=(const ProtocolHandler&) = delete;
};