#include "face_verification.h"

std::string verify_correct_face(
    const std::map<std::string, float>& face_square_normalized_points,
    TranslationManager* translator,
    bool glasses,
    float percentage_min_face_width,
    float percentage_max_face_width,
    float percentage_min_face_height,
    float percentage_max_face_height,
    float percentage_center_allowed_offset
) {
    const std::string wrong_face_width_message = translator->translate("warning.wrong_face_width_message");
    const std::string wrong_face_height_message = translator->translate("warning.wrong_face_height_message");
    const std::string wrong_face_center_message = translator->translate("warning.wrong_face_center_message");
    const std::string face_not_detected_message = translator->translate("warning.face_not_detected_message");
    const std::string face_with_glasses_message = translator->translate("warning.face_with_glasses_message");

    const char* required_keys[] = { "Top Square", "Left Square", "Right Square", "Bottom Square" };
    for (const auto& key : required_keys) {
        if (face_square_normalized_points.find(key) == face_square_normalized_points.end()) {
            return face_not_detected_message;
        }
    }
    if (glasses) {
        return face_with_glasses_message;
    }
    float topSquare = face_square_normalized_points.at("Top Square");
    float leftSquare = face_square_normalized_points.at("Left Square");
    float rightSquare = face_square_normalized_points.at("Right Square");
    float bottomSquare = face_square_normalized_points.at("Bottom Square");

    if (topSquare < 0 || leftSquare < 0 || rightSquare < 0 || bottomSquare < 0) {
        return face_not_detected_message;
    }
    float face_width = rightSquare - leftSquare;
    float face_height = bottomSquare - topSquare;
    float face_center_x = (rightSquare + leftSquare) / 2.0f;
    float face_center_y = (topSquare + bottomSquare) / 2.0f;

    if (!(percentage_min_face_width <= face_width && face_width <= percentage_max_face_width)) {
        return wrong_face_width_message;
    }
    if (!(percentage_min_face_height <= face_height && face_height <= percentage_max_face_height)) {
        return wrong_face_height_message;
    }
    float center = 0.5f;
    if (!(center - percentage_center_allowed_offset <= face_center_x && face_center_x <= center + percentage_center_allowed_offset) ||
        !(center - percentage_center_allowed_offset <= face_center_y && face_center_y <= center + percentage_center_allowed_offset)) {
        return wrong_face_center_message;
    }
    return "";
}