#pragma once

#include "transport.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <limits>
#include <iostream>
#include <utility>

// Callback for messages containing a single image and JSON
using ImageProcessingCallback = std::function<std::pair<cv::Mat, std::string>(const cv::Mat&)>;

// Callback for messages containing only JSON data
using DataProcessingCallback  = std::function<std::string(const std::string&)>;

// Callback for combined message: vector of (image_id, image) and JSON string
using CombinedProcessingCallback = std::function<
    std::pair<std::vector<std::pair<uint32_t, cv::Mat>>, std::string>(
        const std::vector<std::pair<uint32_t, cv::Mat>>&,
        const std::string&)
    >;

// Message type ID in protocol
enum class ProtocolMessageType : uint8_t {
    Image    = 0x01,
    Json     = 0x02,
    Combined = 0x03, // New: variable number of images + JSON
};

// Diagnostic: how big of a message can be processed
constexpr size_t MAX_FRAME_SIZE = 16 * 1024 * 1024; // 16 MB max for uncompressed frame
constexpr size_t MAX_JSON_SIZE  = 1024 * 1024;      // 1 MB for JSON messages
constexpr size_t MAX_IMAGES_IN_COMBINED = 32;       // Arbitrary, can adjust

// This handler is agnostic to the transport implementation!
// NOTE: The transport pointer is NOT owned by ProtocolHandler.
// The caller must ensure the transport is alive for the life of this handler.
class ProtocolHandler {
public:
    ProtocolHandler(Transport* transport,
                    ImageProcessingCallback img_callback,
                    DataProcessingCallback data_callback,
                    CombinedProcessingCallback combined_callback = nullptr);

    // Serve a client session (handle requests over a connection)
    // Returns false on fatal error/connection closed, true on graceful exit.
    bool serve();

    // Send a single image message (type 0x01)
    bool send_image(const cv::Mat& img);

    // Send a single JSON message (type 0x02)
    bool send_json(const std::string& json);

    // Send a combined message (type 0x03)
    // Each image gets a uint32_t id and is a (id, image) pair.
    bool send_combined(const std::vector<std::pair<uint32_t, cv::Mat>>& images,
                        const std::string& json);
                        
    // Handle one full incoming message; returns false on fatal I/O or protocol error.
    bool handle_one_message(std::string& error_out);

    private:
    Transport* m_transport;
    ImageProcessingCallback m_img_cb;
    DataProcessingCallback  m_data_cb;
    CombinedProcessingCallback m_combined_cb;

    // Helpers for reading/writing exact bytes with size limit
    bool read_exact(void* buf, size_t size);
    bool write_exact(const void* buf, size_t size);


    // Helpers for handling combined messages
    bool send_one_combined_image(uint32_t image_id, const cv::Mat& img);
    bool read_one_combined_image(uint32_t& out_id, cv::Mat& img);

    ProtocolHandler(const ProtocolHandler&) = delete;
    ProtocolHandler& operator=(const ProtocolHandler&) = delete;
};