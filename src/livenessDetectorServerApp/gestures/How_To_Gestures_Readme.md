
  ---

  # Gestures JSON File Format

  This document describes how to create and customize *gesture definition* JSON files for use with the Gesture Detector system.

  ---

  ## Table of Contents

  1. [Overview](#overview)  
  2. [File Structure](#file-structure)  
  3. [Gesture Header Fields](#gesture-header-fields)  
  4. [Instructions](#instructions)  
      - [Threshold (Step) Instructions](#threshold-step-instructions)
      - [Range (Hold) Instructions](#range-hold-instructions)
  5. [Examples](#examples)  
  6. [Tips & Best Practices](#tips--best-practices)  
  7. [Schema (Summary)](#schema-summary)

  ---

  ## Overview

  A *gesture* is defined as a sequence of steps (called "instructions") that are checked against some signal (e.g., facial blendshape, eye blink, or any other value). If all instructions are completed in order, the gesture is detected.

  Gestures are stored in JSON files, usually one file per gesture.

  ---

  ## File Structure

  A typical gesture file looks like this:

  ```json
  {
    "gestureId": "unique_id_string",
    "label": "Display name",
    "icon_path": "relative/path/image.png",
    "signal_key": "some_signal",
    "total_recommended_max_time": 10000,
    "take_picture_at_the_end": false,
    "instructions": [
      /* One or more instruction objects */
    ]
  }
  ```

  **OR, using signal_index:**

  ```json
  {
    ...
    "signal_index": 1,
    // instead of "signal_key"
    ...
  }
  ```

  > **Note:**  
  > You should use either `signal_key` or `signal_index`, not both.

  ---

  ## Gesture Header Fields

  | Field                         | Type      | Required | Description                                                   |
  |-------------------------------|-----------|----------|---------------------------------------------------------------|
  | `gestureId`                   | string    | yes      | Unique identifier for the gesture                             |
  | `label`                       | string    | yes      | Display label                                                 |
  | `icon_path`                   | string    | yes      | Path to an icon/image (can be empty string)                   |
  | `signal_key` / `signal_index` | string/int| yes      | Signal to watch (key or index, only one allowed per file)     |
  | `total_recommended_max_time`  | integer   | yes      | Max time to complete gesture (ms)                             |
  | `take_picture_at_the_end`     | boolean   | yes      | Should take snapshot when gesture completes                   |
  | `instructions`                | array     | yes      | A sequence of instruction steps; see below                    |

  ---

  ## Instructions

  The `instructions` field is an array of **instruction objects**.  
  Each instruction can be one of two types: **Threshold** (original behavior) or **Range** ("hold in range" feature).

  Instructions must be completed in order.

  ---

  ### Threshold (Step) Instructions

  **Threshold instructions** are for "cross this limit" checks.  
  You define which direction the threshold should be crossed and the target value.

  #### Fields:

  | Field                 | Type      | Required | Description                                           |
  |-----------------------|-----------|----------|-------------------------------------------------------|
  | `instruction_type`    | string    | *no*     | Must be `"threshold"` (optional; default if omitted)  |
  | `move_to_next_type`   | string    | yes      | `"higher"` or `"lower"`; comparison direction         |
  | `value`               | number    | yes      | Threshold value to compare against                    |
  | `reset`               | object    | yes      | Condition to reset the entire gesture sequence        |

  - **If `move_to_next_type` is `"higher"`:** Step passes when signal goes above `value`.
  - **If `move_to_next_type` is `"lower"`:** Step passes when signal goes below `value`.

  #### Example:

  ```json
  {
    "move_to_next_type": "lower",
    "value": 0.35,
    "reset": {
      "type": "timeout_after_ms", "value": 10000
    }
  }
  ```

  > Omitting `"instruction_type"` field means threshold is assumed (for backwards compatibility).

  ---

  ### Range (Hold) Instructions

  **Range instructions** require the signal to be kept between two values for a set duration.

  #### Fields:

  | Field               | Type      | Required | Description                                    |
  |---------------------|-----------|----------|------------------------------------------------|
  | `instruction_type`  | string    | yes      | `"range"`                                      |
  | `min_value`         | number    | yes      | Minimum allowed value (inclusive)              |
  | `max_value`         | number    | yes      | Maximum allowed value (inclusive)              |
  | `min_duration_ms`   | integer   | yes      | Hold duration (milliseconds)                   |
  | `reset`             | object    | yes      | Condition to reset the entire gesture sequence |

  - The user must keep the signal strictly within `[min_value, max_value]` for at least `min_duration_ms` milliseconds, without leaving the range.

  #### Example:

  ```json
  {
    "instruction_type": "range",
    "min_value": 2.0,
    "max_value": 4.0,
    "min_duration_ms": 2000,
    "reset": {
      "type": "timeout_after_ms",
      "value": 5000
    }
  }
  ```

  ---

  ## Examples

  ### Example 1: Blink (Threshold Gesture)

  ```json
  {
      "gestureId": "blink",
      "label": "Blink your eyes",
      "icon_path": "",
      "signal_key": "eyeBlinkLeft",
      "total_recommended_max_time": 10000,
      "take_picture_at_the_end": true,
      "instructions": [
          {
              "move_to_next_type": "lower",
              "value": 0.35,
              "reset": {"type": "timeout_after_ms", "value": 10000}
          },
          {
              "move_to_next_type": "higher",
              "value": 0.5,
              "reset": {"type": "timeout_after_ms", "value": 10000}
          },
          {
              "move_to_next_type": "lower",
              "value": 0.35,
              "reset": {"type": "timeout_after_ms", "value": 10000}
          }
      ]
  }
  ```
  *(No `instruction_type` fields needed for simple threshold-based steps.)*

  ---

  ### Example 2: Stay Neutral for 2s (Range/Hold Gesture)

  ```json
  {
      "gestureId": "stay_neutral",
      "label": "Keep a neutral face",
      "icon_path": "",
      "signal_key": "browDownLeft",
      "total_recommended_max_time": 5000,
      "take_picture_at_the_end": true,
      "instructions": [
          {
              "instruction_type": "range",
              "min_value": 0.0,
              "max_value": 0.25,
              "min_duration_ms": 2000,
              "reset": {"type": "timeout_after_ms", "value": 5000}
          }
      ]
  }
  ```

  ---

  ## Tips & Best Practices

  - **Only add `"instruction_type": "range"` for ranges; if left out, steps are considered threshold steps.**
  - Avoid `"reset": null`; simply omit the field if there is no reset condition.
  - Choose `signal_key` or `signal_index` based on how your signals are named in your input data.
  - Set `total_recommended_max_time` slightly higher than the expected time to perform the gesture.
  - Use expressive `label` and `gestureId` strings for easier debugging and UI.
  - Always test your JSON with your app after changes for correct behavior.

  ---

  ## Schema (Summary)

  **Threshold step**:
  ```json
  {
    "instruction_type": "threshold", // (optional)
    "move_to_next_type": "higher" | "lower",
    "value": <number>,
    "reset": {
      "type": "lower" | "higher" | "timeout_after_ms",
      "value": <number>
    }
  }
  ```

  **Range/Hold step**:
  ```json
  {
    "instruction_type": "range",
    "min_value": <number>,
    "max_value": <number>,
    "min_duration_ms": <integer>,
    "reset": {
      "type": "lower" | "higher" | "timeout_after_ms",
      "value": <number>
    }
  }
  ```

  **Gesture file:**
  ```json
  {
    "gestureId": "...",
    "label": "...",
    "icon_path": "...",
    "signal_key": "..." | // or "signal_index": ...,
    "total_recommended_max_time": 10000,
    "take_picture_at_the_end": false,
    "instructions": [ { ... }, ... ]
  }
