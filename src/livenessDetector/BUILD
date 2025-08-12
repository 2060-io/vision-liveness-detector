cc_library(
    name = "gesture",
    srcs = ["gesture.cc"],
    hdrs = ["gesture.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "nlohmann",
    hdrs = ["nlohmann/json.hpp"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gesture_detector",
    srcs = ["gesture_detector.cc"],
    hdrs = ["gesture_detector.h"],
    deps = [":gesture", ":nlohmann"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "translation_manager",
    srcs = ["translation_manager.cc"],
    hdrs = ["translation_manager.h"],
    deps = [":nlohmann"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gestures_requester",
    srcs = ["gestures_requester.cc"],
    hdrs = ["gestures_requester.h"],
    deps = [
        ":gesture",
        ":gesture_detector",
        ":translation_manager",
        "//third_party:opencv",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "face_processor",
    srcs = ["face_processor.cc"],
    hdrs = ["face_processor.h"],
    deps = [
        "//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/framework/formats:image_frame_opencv",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "liveness_detector",
    srcs = ["liveness_detector.cc"],
    hdrs = ["liveness_detector.h"],
    deps = [
        ":face_processor",
        ":gestures_requester",
        ":translation_manager",
        ":gesture_detector",
        "//third_party:opencv",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "transport",
    hdrs = ["transport.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "unix_socket_transport",
    srcs = ["unix_socket_transport.cc"],
    hdrs = ["unix_socket_transport.h"],
    deps = [
        ":transport",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "unix_socket_transport_test",
    srcs = ["unix_socket_transport_test.cc"],
    deps = [
        ":unix_socket_transport",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "protocol_handler",
    srcs = ["protocol_handler.cc"],
    hdrs = ["protocol_handler.h"],
    deps = [
        ":transport",
        "//third_party:opencv",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "glasses_detector",
    srcs = ["glasses_detector.cc"],
    hdrs = ["glasses_detector.h"],
    deps = [
        "//third_party:opencv",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "face_verification",
    srcs = ["face_verification.cc"],
    hdrs = ["face_verification.h"],
    deps = [
        ":translation_manager",
         "//third_party:opencv",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "protocol_handler_test",
    srcs = ["protocol_handler_test.cc"],
    deps = [
        ":protocol_handler",
        "@com_google_googletest//:gtest_main",
        "//third_party:opencv",
    ],
    copts = ["-std=c++17"],
)

cc_binary(
    name = "gesture_test",
    srcs = ["gesture_test.cc"],
    deps = [":gesture"],
)

cc_binary(
    name = "gesture_detector_test",
    srcs = ["gesture_detector_test.cc"],
    deps = [
        ":gesture",
        ":gesture_detector",
    ],
)

cc_binary(
    name = "gestures_requester_test",
    srcs = ["gestures_requester_test.cc"],
    deps = [
        ":gesture",
        ":gesture_detector",
        ":translation_manager",
        ":gestures_requester",
        "//third_party:opencv", 
    ],
)

cc_binary(
    name = "translation_manager_test",
    srcs = ["translation_manager_test.cc"],
    deps = [":translation_manager"],
)

cc_binary(
    name = "face_processor_test",
    srcs = ["face_processor_test.cc"],
    deps = [
        ":face_processor",    
        "//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
    ],
    visibility = ["//visibility:public"],
)



