cc_binary(
    name = "livenessDetectorServer",
    srcs = ["livenessDetectorServer.cc"],
    deps = [
        "//livenessDetector:glasses_detector",
        "//livenessDetector:face_verification",
        "//livenessDetector:unix_socket_transport",
        "//livenessDetector:protocol_handler",
        "//livenessDetector:gesture_detector",
        "//livenessDetector:gestures_requester",
        "//livenessDetector:translation_manager",
        "//livenessDetector:face_processor",
        "//livenessDetector:nlohmann",
        "//third_party:opencv",
    ],
    copts = ["-std=c++17"],       # enable C++17 for <filesystem>
    linkopts = select({
        "@platforms//os:linux": ["-lstdc++fs"],
        "@platforms//os:windows": [],
        "@platforms//os:osx": [],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
