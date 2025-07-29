#include "protocol_handler.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

ProtocolHandler::ProtocolHandler(
    Transport* transport,
    ImageProcessingCallback img_callback,
    DataProcessingCallback data_callback
)
    : m_transport(transport), m_img_cb(img_callback), m_data_cb(data_callback) {}

bool ProtocolHandler::read_exact(void* buf, size_t size) {
    return m_transport->read_exact(buf, size);
}
bool ProtocolHandler::write_exact(const void* buf, size_t size) {
    return m_transport->write_exact(buf, size);
}

bool ProtocolHandler::send_image(const cv::Mat& img) {
    uint8_t fid = 0x01;
    uint32_t frame_size = img.total() * img.elemSize();
    uint32_t net_size = htonl(frame_size);
    uint32_t net_rows = htonl(img.rows);
    uint32_t net_cols = htonl(img.cols);

    if (!write_exact(&fid, 1)) return false;
    if (!write_exact(&net_size, 4)) return false;
    if (!write_exact(&net_rows, 4)) return false;
    if (!write_exact(&net_cols, 4)) return false;
    if (!write_exact(img.data, frame_size)) return false;
    return true;
}

bool ProtocolHandler::send_json(const std::string& json) {
    uint8_t fid = 0x02;
    uint32_t string_size = htonl(static_cast<uint32_t>(json.size()));
    if (!write_exact(&fid, 1)) return false;
    if (!write_exact(&string_size, 4)) return false;
    if (!write_exact(json.data(), json.size())) return false;
    return true;
}

void ProtocolHandler::serve() {
    while (handle_one_message()) {}
}

bool ProtocolHandler::handle_one_message() {
    uint8_t fid = 0;
    if (!read_exact(&fid, 1)) {
        std::cerr << "Failed to read function ID (possibly disconnect)\n";
        return false;
    }
    if (fid == 0x01) { // image
        uint32_t frame_net, rows_net, cols_net;
        if (!read_exact(&frame_net, 4) || !read_exact(&rows_net, 4) || !read_exact(&cols_net, 4))
            return false;
        uint32_t frame_size = ntohl(frame_net);
        uint32_t rows = ntohl(rows_net);
        uint32_t cols = ntohl(cols_net);

        std::vector<uint8_t> frame_buf(frame_size);
        if (!read_exact(frame_buf.data(), frame_size)) return false;

        cv::Mat img(rows, cols, CV_8UC3, frame_buf.data());
        auto [out_img, info_json] = m_img_cb(img);

        if (!info_json.empty()) {
            if (!send_json(info_json)) {
                std::cerr << "Failed to send JSON response\n";
                return false;
            }
        }
        if (!out_img.empty()) {
            if (!send_image(out_img)) {
                std::cerr << "Failed to send image response\n";
                return false;
            }
        }
    } else if (fid == 0x02) { // JSON/config request
        uint32_t json_size_net;
        if (!read_exact(&json_size_net, 4)) return false;
        uint32_t json_size = ntohl(json_size_net);

        std::vector<char> json_buf(json_size);
        if (!read_exact(json_buf.data(), json_size)) return false;
        std::string json_in(json_buf.data(), json_size);
        std::string json_out = m_data_cb(json_in);

        if (!json_out.empty()) {
            if (!send_json(json_out)) {
                std::cerr << "Failed to send JSON reply\n";
                return false;
            }
        }
    } else {
        std::cerr << "Unknown function ID " << static_cast<int>(fid) << "\n";
        return false;
    }
    return true;
}