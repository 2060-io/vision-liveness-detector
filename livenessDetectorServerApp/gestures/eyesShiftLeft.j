{
    "gestureId": "eyesShiftLeft",
    "label": "Look at the camera, then shift your EYES to the LEFT , then return to the camera",
    "icon_path": "./gestures/icons/sample.png",
    "signal_key": "eyeLookOutLeft",
    "total_recommended_max_time":10000,
    "take_picture_at_the_end": true,
    "instructions": [
        {
            "move_to_next_type": "lower",
            "value": 0.3,
            "reset": {
                "type": "timeout_after_ms",
                "value": 10000
            }
        },
        {
            "move_to_next_type": "higher",
            "value": 0.5,
            "reset": {
                "type": "timeout_after_ms",
                "value": 10000
            }
        },
        {
            "move_to_next_type": "lower",
            "value": 0.3,
            "reset": {
                "type": "timeout_after_ms",
                "value": 10000
            }
        }
    ]
}