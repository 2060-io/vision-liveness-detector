gesturesSchema = {
    "type": "object",
    "properties": {
        "gestureId": {"type": "string"},
        "label": {"type": "string"},
        "icon_path": {"type": "string"},
        "signal_index": {"type": "integer"},
        "signal_key": {"type": "string"},
        "total_recommended_max_time": {"type": "integer"},
        "take_picture_at_the_end": {"type": "boolean"},
        "instructions": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "move_to_next_type": {"type": "string"},
                    "value": {"type": "number"},
                    "reset": {
                        "type": "object",
                        "properties": {
                            "type": {"type": "string"},
                            "value": {"type": "integer"}
                        },
                        "required": ["type", "value"]
                    }
                },
                "required": ["move_to_next_type", "value", "reset"]
            }
        }
    },
    "oneOf": [
        {"required": ["signal_index"], "not": {"required": ["signal_key"]}},
        {"required": ["signal_key"], "not": {"required": ["signal_index"]}}
    ],
    "required": ["gestureId", "label", "icon_path", "total_recommended_max_time", "take_picture_at_the_end", "instructions"]
}
