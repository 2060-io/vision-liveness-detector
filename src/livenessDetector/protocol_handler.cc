#include "protocol_handler.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <cassert>

// Helper: clamp value to a maximum allowed
template <typename A, typename B>
bool clamp_max(A val, B maxval) {
    return val <= maxval;
}

ProtocolHandler::ProtocolHandler(
    Transport* transport,
    ImageProcessingCallback img_callback,
    DataProcessingCallback data_callback,
    CombinedProcessingCallback combined_callback
)
    : m_transport(transport),
      m_img_cb(img_callback),
      m_data_cb(data_callback),
      m_combined_cb(combined_callback)
{}

// Safely forward to underlying transport
bool ProtocolHandler::read_exact(void* buf, size_t size) {
    return m_transport->read_exact(buf, size);
}
bool ProtocolHandler::write_exact(const void* buf, size_t size) {
    return m_transport->write_exact(buf, size);
}

// Send: outbound image (as raw bytes)
bool ProtocolHandler::send_image(const cv::Mat& img) {
    const ProtocolMessageType fid = ProtocolMessageType::Image;
    const uint32_t rows = static_cast<uint32_t>(img.rows);
    const uint32_t cols = static_cast<uint32_t>(img.cols);

    // Compute payload size safely
    size_t frame_size = img.total() * img.elemSize();
    if (!clamp_max(frame_size, MAX_FRAME_SIZE)) {
        std::cerr << "send_image: frame too large: " << frame_size << " > " << MAX_FRAME_SIZE << "\n";
        return false;
    }
    uint32_t net_size = htonl(static_cast<uint32_t>(frame_size));
    uint32_t net_rows = htonl(rows);
    uint32_t net_cols = htonl(cols);

    if (!write_exact(&fid, 1)) return false;
    if (!write_exact(&net_size, 4)) return false;
    if (!write_exact(&net_rows, 4)) return false;
    if (!write_exact(&net_cols, 4)) return false;
    if (!write_exact(img.data, frame_size)) return false;
    return true;
}

// Send: outbound JSON result
bool ProtocolHandler::send_json(const std::string& json) {
    const ProtocolMessageType fid = ProtocolMessageType::Json;
    size_t string_size = json.size();
    if (!clamp_max(string_size, MAX_JSON_SIZE)) {
        std::cerr << "send_json: JSON size too large: " << string_size << " > " << MAX_JSON_SIZE << "\n";
        return false;
    }
    uint32_t net_size = htonl(static_cast<uint32_t>(string_size));
    if (!write_exact(&fid, 1)) return false;
    if (!write_exact(&net_size, 4)) return false;
    if (!write_exact(json.data(), string_size)) return false;
    return true;
}

// =========== NEW: SEND COMBINED (0x03) ===========
bool ProtocolHandler::send_combined(const std::vector<std::pair<uint32_t, cv::Mat>>& images,
                                    const std::string& json)
{
    const ProtocolMessageType fid = ProtocolMessageType::Combined;

    if (images.size() > MAX_IMAGES_IN_COMBINED) {
        std::cerr << "send_combined: too many images: " << images.size() << " > " << MAX_IMAGES_IN_COMBINED << "\n";
        return false;
    }

    // Compute total size for validation (optional, let's check JSON length at least)
    if (!clamp_max(json.size(), MAX_JSON_SIZE)) {
        std::cerr << "send_combined: JSON too large\n";
        return false;
    }

    uint32_t images_count = static_cast<uint32_t>(images.size());
    uint32_t net_images_count = htonl(images_count);

    if (!write_exact(&fid, 1)) return false;
    if (!write_exact(&net_images_count, 4)) return false;

    // Write each image
    for (const auto& image_pair : images) {
        if (!send_one_combined_image(image_pair.first, image_pair.second)) {
            std::cerr << "send_combined: failed to write one image block\n";
            return false;
        }
    }

    // Write JSON section
    uint32_t json_size = static_cast<uint32_t>(json.size());
    uint32_t net_json_size = htonl(json_size);
    if (!write_exact(&net_json_size, 4)) return false;
    if (!write_exact(json.data(), json_size)) return false;

    return true;
}

// Helper: send a block for one image in Combined
bool ProtocolHandler::send_one_combined_image(uint32_t image_id, const cv::Mat& img)
{
    uint32_t rows = static_cast<uint32_t>(img.rows);
    uint32_t cols = static_cast<uint32_t>(img.cols);
    size_t frame_size = img.total() * img.elemSize();

    if (!clamp_max(frame_size, MAX_FRAME_SIZE)) {
        std::cerr << "send_one_combined_image: frame too large\n";
        return false;
    }

    uint32_t net_id   = htonl(image_id);
    uint32_t net_size = htonl(static_cast<uint32_t>(frame_size));
    uint32_t net_rows = htonl(rows);
    uint32_t net_cols = htonl(cols);

    if (!write_exact(&net_id, 4))   { std::cerr << "[send_one_combined_image] Failed to write image id\n"; return false; }
    if (!write_exact(&net_size, 4)) { std::cerr << "[send_one_combined_image] Failed to write image size\n"; return false; }
    if (!write_exact(&net_rows, 4)) { std::cerr << "[send_one_combined_image] Failed to write image rows\n"; return false; }
    if (!write_exact(&net_cols, 4)) { std::cerr << "[send_one_combined_image] Failed to write image cols\n"; return false; }
    if (!write_exact(img.data, frame_size)) { std::cerr << "[send_one_combined_image] Failed to write image data\n"; return false; }
    return true;
}

bool ProtocolHandler::serve() {
    std::string why_exit;
    while (handle_one_message(why_exit)) {}
    std::cerr << "Session ended: " << why_exit << "\n";
    return false;
}

// Helper: Read one combined image block into id + cv::Mat
bool ProtocolHandler::read_one_combined_image(uint32_t& out_id, cv::Mat& img)
{
    uint32_t net_id = 0, net_size = 0, net_rows = 0, net_cols = 0;
    if (!read_exact(&net_id, 4)) return false;
    if (!read_exact(&net_size, 4)) return false;
    if (!read_exact(&net_rows, 4)) return false;
    if (!read_exact(&net_cols, 4)) return false;
    out_id = ntohl(net_id);
    uint32_t frame_size = ntohl(net_size);
    uint32_t rows = ntohl(net_rows);
    uint32_t cols = ntohl(net_cols);
    if (!clamp_max(frame_size, MAX_FRAME_SIZE)) return false;
    if (rows == 0 || cols == 0 || rows > 10000 || cols > 10000) return false;
    if ((frame_size != rows * cols * 3)) return false;
    std::vector<uint8_t> buf(frame_size);
    if (!read_exact(buf.data(), frame_size)) return false;
    img = cv::Mat(rows, cols, CV_8UC3, buf.data()).clone();
    return true;
}

// Accepts an error log string reference for reporting reason
bool ProtocolHandler::handle_one_message(std::string& error_out) {
    uint8_t fid_raw = 0;
    if (!read_exact(&fid_raw, 1)) {
        error_out = "Failed to read function ID (likely disconnect)";
        return false;
    }

    ProtocolMessageType fid = static_cast<ProtocolMessageType>(fid_raw);

    if (fid == ProtocolMessageType::Image) {
        uint32_t frame_net, rows_net, cols_net;
        if (!read_exact(&frame_net, 4) || !read_exact(&rows_net, 4) || !read_exact(&cols_net, 4)) {
            error_out = "Failed to read image header";
            return false;
        }
        uint32_t frame_size = ntohl(frame_net);
        uint32_t rows = ntohl(rows_net);
        uint32_t cols = ntohl(cols_net);

        if (!clamp_max(frame_size, MAX_FRAME_SIZE)) {
            error_out = "Frame too big, rejecting";
            return false;
        }
        if (rows == 0 || cols == 0 || rows > 10000 || cols > 10000) {
            error_out = "Bogus rows/cols in image header";
            return false;
        }
        // Only support 3-channel 8-bit BGR images (CV_8UC3)
        if ((frame_size != rows * cols * 3)) {
            error_out = "Mismatch of frame_size vs image dimensions";
            return false;
        }

        std::vector<uint8_t> frame_buf(frame_size);
        if (!read_exact(frame_buf.data(), frame_size)) {
            error_out = "Failed to read image payload";
            return false;
        }
        cv::Mat img(rows, cols, CV_8UC3, frame_buf.data());
        auto [out_img, info_json] = m_img_cb(img);

        if (!info_json.empty()) {
            if (!send_json(info_json)) {
                error_out = "Failed to send JSON response";
                return false;
            }
        }
        if (!out_img.empty()) {
            if (!send_image(out_img)) {
                error_out = "Failed to send image response";
                return false;
            }
        }
    } else if (fid == ProtocolMessageType::Json) {
        uint32_t json_size_net;
        if (!read_exact(&json_size_net, 4)) {
            error_out = "Failed to read JSON header";
            return false;
        }
        uint32_t json_size = ntohl(json_size_net);

        if (!clamp_max(json_size, MAX_JSON_SIZE)) {
            error_out = "JSON frame too large";
            return false;
        }
        std::vector<char> json_buf(json_size);
        if (!read_exact(json_buf.data(), json_size)) {
            error_out = "Failed to read JSON payload";
            return false;
        }
        std::string json_in(json_buf.data(), json_size);
        std::string json_out = m_data_cb(json_in);

        if (!json_out.empty()) {
            if (!send_json(json_out)) {
                error_out = "Failed to send JSON reply";
                return false;
            }
        }
    } else if (fid == ProtocolMessageType::Combined) {
        // ===== NEW: COMBINED MESSAGE HANDLING =====
        uint32_t net_images_count = 0;
        if (!read_exact(&net_images_count, 4)) {
            error_out = "Failed to read combined image count";
            return false;
        }
        uint32_t images_count = ntohl(net_images_count);
        if (images_count > MAX_IMAGES_IN_COMBINED) {
            error_out = "Too many images in combined packet";
            return false;
        }

        std::vector<std::pair<uint32_t, cv::Mat>> images;
        for (uint32_t i = 0; i < images_count; ++i) {
            uint32_t id = 0;
            cv::Mat img;
            if (!read_one_combined_image(id, img)) {
                error_out = "Failed to read combined image block";
                return false;
            }
            images.emplace_back(id, img);
        }

        uint32_t net_json_size = 0;
        if (!read_exact(&net_json_size, 4)) {
            error_out = "Failed to read combined JSON size";
            return false;
        }
        uint32_t json_size = ntohl(net_json_size);
        if (!clamp_max(json_size, MAX_JSON_SIZE)) {
            error_out = "Combined: JSON size too large";
            return false;
        }
        std::vector<char> json_buf(json_size);
        if (!read_exact(json_buf.data(), json_size)) {
            error_out = "Failed to read combined JSON";
            return false;
        }
        std::string json_str(json_buf.data(), json_size);

        if (m_combined_cb) {
            auto [out_images, out_json] = m_combined_cb(images, json_str);
            if (!out_json.empty()) {
                if (!send_json(out_json)) {
                    error_out = "Failed to send JSON response for combined";
                    return false;
                }
            }
            if (!out_images.empty()) {
                if (!send_combined(out_images, "")) { // If reply has images and no JSON needed
                    error_out = "Failed to send combined image response";
                    return false;
                }
            }
        } else {
            // No handler, just ignore
        }
    } else {
        error_out = "Unknown function ID: " + std::to_string(fid_raw);
        return false;
    }
    return true;
}