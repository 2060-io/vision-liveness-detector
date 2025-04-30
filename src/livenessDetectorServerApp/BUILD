cc_binary(
    name = "livenessDetectorServer",
    srcs = ["livenessDetectorServer.cc"],
    deps = [
        "//livenessDetector:unix_socket_server",
        "//livenessDetector:gesture_detector",
        "//livenessDetector:gestures_requester",
        "//livenessDetector:translation_manager",
        "//livenessDetector:face_processor",
        "//livenessDetector:nlohmann",
        "//third_party:opencv",
    ],
    copts   = ["-std=c++17"],       # enable C++17 for <filesystem>
    linkopts = ["-lstdc++fs"],      # pull in the filesystem library
    visibility = ["//visibility:public"],
)
