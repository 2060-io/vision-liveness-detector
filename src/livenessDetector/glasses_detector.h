#pragma once

#include <opencv2/core.hpp>
#include <string>

// Forward declaration instead of full #include for translation_manager.
class GlassesDetectorManager;

// Enum for glasses detection mode.
enum class GlassesMode { OFF, WARNING_ONLY, ERROR };

// Parses a string like "OFF", "WARNING_ONLY", "ERROR" to GlassesMode.
GlassesMode parse_glasses_mode(const std::string& mode);

// Detects glasses in a sequence of face images.
class GlassesDetectorManager {
public:
    explicit GlassesDetectorManager(const std::string& model_path, int filter_frames = 30);
    ~GlassesDetectorManager();  // ADD THIS LINE
    bool is_loaded() const;
    bool detect_and_update(const cv::Mat& image);

private:
    struct Impl;
    Impl* pImpl;
};