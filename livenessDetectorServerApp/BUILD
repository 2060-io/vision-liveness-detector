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
    visibility = ["//visibility:public"],
)
