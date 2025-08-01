#include "protocol_handler.h"
#include <gtest/gtest.h>
#include <queue>
#include <cstring>

class FakeTransport : public Transport {
public:
    std::queue<std::vector<uint8_t>> input_chunks;
    std::vector<uint8_t> written;
    bool fail_next_read = false;
    bool fail_next_write = false;
    bool closed = false;

    bool open_server() override { return true; }
    bool open_client() override { return true; }
    void close() noexcept override { closed = true; }
    bool is_open() const override { return !closed; }
    int get_fd() const override { return 42; }

    // Read exact, will pop input_chunks as input
    bool read_exact(void* buffer, size_t size) override {
        if (fail_next_read) return false;
        if (input_chunks.empty()) return false;
        auto& chunk = input_chunks.front();
        if (chunk.size() < size) return false;
        std::memcpy(buffer, chunk.data(), size);
        chunk.erase(chunk.begin(), chunk.begin()+size);
        if (chunk.empty()) input_chunks.pop();
        return true;
    }
    bool write_exact(const void* buffer, size_t size) override {
        if (fail_next_write) return false;
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(buffer);
        written.insert(written.end(), ptr, ptr+size);
        return true;
    }
};

// Helper to encode 32-bit network order
inline void encode_uint32(std::vector<uint8_t>& out, uint32_t v) {
    uint32_t netv = htonl(v);
    uint8_t* p = reinterpret_cast<uint8_t*>(&netv);
    out.insert(out.end(), p, p+4);
}

// Helper to build a [img_id|size|rows|cols|data] block for combined messages
void append_combined_image_block(std::vector<uint8_t>& buf, uint32_t img_id, const cv::Mat& img) {
    size_t data_len = img.total() * img.elemSize();
    encode_uint32(buf, img_id);
    encode_uint32(buf, static_cast<uint32_t>(data_len));
    encode_uint32(buf, static_cast<uint32_t>(img.rows));
    encode_uint32(buf, static_cast<uint32_t>(img.cols));
    buf.insert(buf.end(), img.data, img.data + data_len);
}

TEST(ProtocolHandlerTest, ValidImageAndJson) {
    FakeTransport t;

    // Prepare image message: fid | size | rows | cols | payload (one blue pixel)
    std::vector<uint8_t> image_msg;
    image_msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Image));
    encode_uint32(image_msg, 3);  // frame size: 1 pixel, 3 channels
    encode_uint32(image_msg, 1);  // 1 row
    encode_uint32(image_msg, 1);  // 1 col
    image_msg.insert(image_msg.end(), {0,0,255});  // BGR blue

    // Prepare JSON message: fid | size | payload ("hello")
    std::vector<uint8_t> json_msg;
    json_msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Json));
    encode_uint32(json_msg, 5);
    for (char c : std::string("hello")) json_msg.push_back(c);

    t.input_chunks.push(image_msg);
    t.input_chunks.push(json_msg);

    bool imageCbCalled = false;
    bool jsonCbCalled = false;

    auto img_cb = [&](const cv::Mat& m) {
        imageCbCalled = true;
        EXPECT_EQ(m.rows, 1);
        EXPECT_EQ(m.cols, 1);
        // Return a response image and empty JSON
        cv::Mat respond = m.clone();
        return std::make_pair(respond, std::string());
    };
    auto json_cb = [&](const std::string& j) {
        jsonCbCalled = true;
        EXPECT_EQ(j, "hello");
        return std::string("{\"ok\":1}");
    };

    ProtocolHandler handler(&t, img_cb, json_cb);
    handler.serve();

    EXPECT_TRUE(imageCbCalled);
    EXPECT_TRUE(jsonCbCalled);

    // Output: two messages -- one image sent, one JSON reply sent
    EXPECT_GT(t.written.size(), 0u);
}

TEST(ProtocolHandlerTest, RejectOversizeImage) {
    FakeTransport t;
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Image));
    encode_uint32(msg, MAX_FRAME_SIZE+1); // frame_size > max
    encode_uint32(msg, 1);
    encode_uint32(msg, 1);
    msg.insert(msg.end(), 3, 123);
    t.input_chunks.push(msg);

    ProtocolHandler handler(&t,
        [](const cv::Mat& m){return std::pair<cv::Mat,std::string>();},
        [](const std::string&){return std::string();});
    handler.serve();
}

TEST(ProtocolHandlerTest, RejectOversizeJson) {
    FakeTransport t;
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Json));
    encode_uint32(msg, MAX_JSON_SIZE+1); // oversize
    msg.resize(msg.size() + (MAX_JSON_SIZE+1), 'a');
    t.input_chunks.push(msg);

    ProtocolHandler handler(&t,
        [](const cv::Mat& m){return std::pair<cv::Mat,std::string>();},
        [](const std::string&){return std::string();});
    handler.serve();
}

TEST(ProtocolHandlerTest, InvalidFunctionID) {
    FakeTransport t;
    std::vector<uint8_t> msg;
    msg.push_back(99);  // Not a valid ProtocolMessageType
    t.input_chunks.push(msg);

    ProtocolHandler handler(&t,
        [](const cv::Mat& m){return std::pair<cv::Mat,std::string>();},
        [](const std::string&){return std::string();});
    handler.serve();
}

TEST(ProtocolHandlerTest, TruncatedImageFailsGracefully) {
    FakeTransport t;
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Image));
    encode_uint32(msg, 6);  // frame_size
    encode_uint32(msg, 1);
    encode_uint32(msg, 2);
    // Missing actual payload (should be 6 bytes)
    t.input_chunks.push(msg);

    ProtocolHandler handler(&t,
        [](const cv::Mat& m){return std::pair<cv::Mat,std::string>();},
        [](const std::string&){return std::string();});
    handler.serve();
}

TEST(ProtocolHandlerTest, MalformedImageHeader) {
    FakeTransport t;
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Image));
    encode_uint32(msg, 3);
    encode_uint32(msg, 0);   // rows = 0
    encode_uint32(msg, 10);  // cols = 10
    msg.resize(msg.size() + 3, 123);
    t.input_chunks.push(msg);

    ProtocolHandler handler(&t,
        [](const cv::Mat& m){return std::pair<cv::Mat,std::string>();},
        [](const std::string&){return std::string();});
    handler.serve();
}

// ===================
// Combined 0x03 tests
// ===================

TEST(ProtocolHandlerTest, CombinedValidTwoImagesAndJson) {
    FakeTransport t;

    cv::Mat red(1,2,CV_8UC3, cv::Scalar(0,0,255));
    cv::Mat green(1,2,CV_8UC3, cv::Scalar(0,255,0));
    uint32_t red_id = 10, green_id = 22;
    std::string json_payload = "{\"msg\":\"hello from combined\"}";

    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Combined));
    encode_uint32(msg, 2); // num images

    append_combined_image_block(msg, red_id, red);
    append_combined_image_block(msg, green_id, green);
    encode_uint32(msg, static_cast<uint32_t>(json_payload.size()));
    msg.insert(msg.end(), json_payload.begin(), json_payload.end());
    t.input_chunks.push(msg);

    bool combinedCbCalled = false;
    ProtocolHandler handler(&t,
        [](const cv::Mat&) { return std::pair<cv::Mat, std::string>(); },
        [](const std::string&) { return std::string(); },
        [&](const std::vector<std::pair<uint32_t, cv::Mat>>& imgs, const std::string& js)
            -> std::pair<std::vector<std::pair<uint32_t, cv::Mat>>, std::string>
        {
            combinedCbCalled = true;
            EXPECT_EQ(imgs.size(), 2u);
            EXPECT_EQ(imgs[0].first, red_id);
            EXPECT_EQ(imgs[1].first, green_id);
            EXPECT_EQ(imgs[0].second.rows, 1);
            EXPECT_EQ(imgs[1].second.cols, 2);
            EXPECT_EQ(js, json_payload);
            return {};
        }
    );
    handler.serve();
    EXPECT_TRUE(combinedCbCalled);
}

TEST(ProtocolHandlerTest, CombinedZeroImagesWithJson) {
    FakeTransport t;

    std::string json_payload = "{\"msg\":\"empty images\"}";
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Combined));
    encode_uint32(msg, 0); // 0 images
    encode_uint32(msg, static_cast<uint32_t>(json_payload.size()));
    msg.insert(msg.end(), json_payload.begin(), json_payload.end());
    t.input_chunks.push(msg);

    bool combinedCbCalled = false;
    ProtocolHandler handler(&t,
        [](const cv::Mat&) { return std::pair<cv::Mat, std::string>(); },
        [](const std::string&) { return std::string(); },
        [&](const std::vector<std::pair<uint32_t, cv::Mat>>& imgs, const std::string& js)
            -> std::pair<std::vector<std::pair<uint32_t, cv::Mat>>, std::string>
        {
            combinedCbCalled = true;
            EXPECT_TRUE(imgs.empty());
            EXPECT_EQ(js, json_payload);
            return {};
        }
    );
    handler.serve();
    EXPECT_TRUE(combinedCbCalled);
}

TEST(ProtocolHandlerTest, CombinedReplyWorks) {
    FakeTransport t;

    cv::Mat im(1,1,CV_8UC3, cv::Scalar(5,6,7));
    std::string json_payload = "{\"foo\":1,\"bar\":2}";
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(ProtocolMessageType::Combined));
    encode_uint32(msg, 1); // 1 image
    append_combined_image_block(msg, 123, im);
    encode_uint32(msg, static_cast<uint32_t>(json_payload.size()));
    msg.insert(msg.end(), json_payload.begin(), json_payload.end());
    t.input_chunks.push(msg);

    ProtocolHandler handler(&t,
        [](const cv::Mat&) { return std::pair<cv::Mat, std::string>(); },
        [](const std::string&) { return std::string(); },
        [&](const std::vector<std::pair<uint32_t, cv::Mat>>& imgs, const std::string& js)
            -> std::pair<std::vector<std::pair<uint32_t, cv::Mat>>, std::string>
        {
            std::vector<std::pair<uint32_t, cv::Mat>> outimgs;
            outimgs.push_back({777, imgs.at(0).second});
            std::string outjson = "{\"ok\":true}";
            return {outimgs, outjson};
        }
    );
    handler.serve();

    bool saw_comb = false, saw_json = false;
    size_t i = 0;
    while (i < t.written.size()) {
        uint8_t fid = t.written[i++];
        if(fid == static_cast<uint8_t>(ProtocolMessageType::Json)) {
            saw_json = true;
            if (i + 4 > t.written.size()) break;
            uint32_t sz = 0;
            std::memcpy(&sz, &t.written[i], 4);
            sz = ntohl(sz);
            i += 4;
            if (i + sz > t.written.size()) break;
            i += sz;
            continue;
        } else if(fid == static_cast<uint8_t>(ProtocolMessageType::Combined)) {
            saw_comb = true;
            break;
        } else {
            break;
        }
    }
    EXPECT_TRUE(saw_comb);
    EXPECT_TRUE(saw_json);
}