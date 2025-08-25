#include "glasses_detector.h"

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <algorithm>
#include <deque>
#include <iostream>
#include <cctype>
#include <vector>
#include <stdexcept>

GlassesMode parse_glasses_mode(const std::string& mode) {
    std::string u(mode);
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    if (u == "OFF") return GlassesMode::OFF;
    if (u == "WARNING_ONLY") return GlassesMode::WARNING_ONLY;
    if (u == "ERROR") return GlassesMode::ERROR;
    return GlassesMode::OFF;
}

struct GlassesDetectorManager::Impl {
    cv::dnn::Net net;
    int filter_size;
    bool model_loaded;
    std::deque<bool> glasses_history;

    Impl(const std::string& model_path, int filter_frames)
    : filter_size(filter_frames), model_loaded(false)
    {
        try {
            net = cv::dnn::readNetFromONNX(model_path);
            model_loaded = !net.empty();
        }
        catch (const std::exception& ex) {
            std::cerr << "[GlassesDetector] Model load error: " << ex.what() << std::endl;
            model_loaded = false;
        }
    }

    bool detect(const cv::Mat& image) {
        if (!model_loaded || image.empty())
            return false;
        cv::Mat input;
        cv::cvtColor(image, input, cv::COLOR_BGR2RGB);
        cv::resize(input, input, cv::Size(160, 160));
        input.convertTo(input, CV_32F);
        cv::Mat nchwBlob = cv::dnn::blobFromImage(input); // (1, 3, 160, 160)
        cv::Mat nhwcBlob = nchw_to_nhwc(nchwBlob);
        net.setInput(nhwcBlob);
        float result = net.forward().at<float>(0, 0);
        return (result < 0.0f);
    }

    static cv::Mat nchw_to_nhwc(const cv::Mat& nchwBlob) {
        CV_Assert(nchwBlob.dims == 4 && nchwBlob.type() == CV_32F);
        int N = nchwBlob.size[0], C = nchwBlob.size[1], H = nchwBlob.size[2], W = nchwBlob.size[3];
        cv::Mat nhwcBlob = cv::Mat::zeros(N, H * W * C, CV_32F);

        const float* src = reinterpret_cast<const float*>(nchwBlob.data);
        float* dst = reinterpret_cast<float*>(nhwcBlob.data);

        for (int n = 0; n < N; ++n)
            for (int h = 0; h < H; ++h)
                for (int w = 0; w < W; ++w)
                    for (int c = 0; c < C; ++c)
                        dst[n * H * W * C + h * W * C + w * C + c] =
                            src[n * C * H * W + c * H * W + h * W + w];

        nhwcBlob = nhwcBlob.reshape(1, std::vector<int>{N, H, W, C});
        return nhwcBlob;
    }
};

GlassesDetectorManager::GlassesDetectorManager(const std::string& model_path, int filter_frames)
: pImpl(new Impl(model_path, filter_frames)) {}

GlassesDetectorManager::~GlassesDetectorManager() { delete pImpl; }

bool GlassesDetectorManager::is_loaded() const { return pImpl->model_loaded; }

bool GlassesDetectorManager::detect_and_update(const cv::Mat& image) {
    bool is_glasses = pImpl->detect(image);
    pImpl->glasses_history.push_back(is_glasses);
    if ((int)pImpl->glasses_history.size() > pImpl->filter_size)
        pImpl->glasses_history.pop_front();

    int true_count = std::count(pImpl->glasses_history.begin(), pImpl->glasses_history.end(), true);
    int false_count = (int)pImpl->glasses_history.size() - true_count;
    return (true_count > false_count);
}