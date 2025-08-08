# Gestures JSON File Format

This document describes how to create and customize **gesture definition** JSON files for use with the Gesture Detector system.

---

## Table of Contents

1. [Overview](#overview)  
2. [File Structure](#file-structure)  
3. [Gesture Header Fields](#gesture-header-fields)  
4. [Instructions](#instructions)  
    - [Threshold (Step) Instructions](#threshold-step-instructions)
    - [Range (Hold) Instructions](#range-hold-instructions)
5. [Picture-Taking Options](#picture-taking-options)  
6. [Examples](#examples)  
7. [Tips & Best Practices](#tips--best-practices)  
8. [Schema (Summary)](#schema-summary)

---

## Overview

A *gesture* is defined as a sequence of steps (called “instructions”) that are checked against some signal (e.g., facial blendshape, eye blink, or any other value).  
If all instructions are completed in order, the gesture is detected.

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
    "randomize_step_picture": false,
    "instructions": [
    /* One or more instruction objects */
    ]
}
```

**OR, using `signal_index`:**

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

| Field                         | Type      | Required | Description                                                     |
|-------------------------------|-----------|----------|-----------------------------------------------------------------|
| `gestureId`                   | string    | yes      | Unique identifier for the gesture                               |
| `label`                       | string    | yes      | Display label                                                   |
| `icon_path`                   | string    | yes      | Path to an icon/image (can be empty string)                     |
| `signal_key` / `signal_index` | string/int| yes      | Signal to watch (key or index, only one allowed per file)       |
| `total_recommended_max_time`  | integer   | yes      | Max time to complete gesture (ms)                               |
| `take_picture_at_the_end`     | boolean   | yes      | Should take snapshot when gesture completes                     |
| `randomize_step_picture`      | boolean   | no       | If true, only one random step marked for photo will trigger it  |
| `instructions`                | array     | yes      | Sequence of instruction steps — see below                       |

---

## Instructions

The `instructions` field is an array of **instruction objects**.  
Each instruction can be one of two types: **Threshold** (original behavior) or **Range** (“hold in range” feature).

Instructions must be completed in order.

Each instruction also supports some **optional** fields:

| Field                    | Type      | Required | Description                                                   |
|--------------------------|-----------|----------|---------------------------------------------------------------|
| `take_picture_at_the_end`| boolean   | no       | If true, triggers picture-callback at the end of this step    |

---

### Threshold (Step) Instructions

**Threshold instructions** are for “cross this limit” checks.  
You define which direction the threshold should be crossed and the target value.

#### Fields

| Field                 | Type      | Required | Description                                           |
|-----------------------|-----------|----------|-------------------------------------------------------|
| `instruction_type`    | string    | *no*     | Must be `"threshold"` (optional; default if omitted)  |
| `move_to_next_type`   | string    | yes      | `"higher"` or `"lower"`; comparison direction         |
| `value`               | number    | yes      | Threshold value to compare against                    |
| `reset`               | object    | yes      | Condition to reset the entire gesture sequence        |
| `take_picture_at_the_end` | boolean | no      | If true, triggers photo-callback at end of this step  |

#### Example

```json
{
    "move_to_next_type": "lower",
    "value": 0.35,
    "reset": {
    "type": "timeout_after_ms", "value": 10000
    },
    "take_picture_at_the_end": true
}
```

---

### Range (Hold) Instructions

**Range instructions** require the signal to be kept between two values for a set duration.

#### Fields

| Field                    | Type      | Required | Description                                    |
|--------------------------|-----------|----------|------------------------------------------------|
| `instruction_type`       | string    | yes      | `"range"`                                      |
| `min_value`              | number    | yes      | Minimum allowed value (inclusive)              |
| `max_value`              | number    | yes      | Maximum allowed value (inclusive)              |
| `min_duration_ms`        | integer   | yes      | Hold duration (milliseconds)                   |
| `reset`                  | object    | yes      | Condition to reset the entire gesture sequence |
| `take_picture_at_the_end`| boolean   | no       | If true, triggers photo-callback at this step  |

#### Example

```json
{
    "instruction_type": "range",
    "min_value": 2.0,
    "max_value": 4.0,
    "min_duration_ms": 2000,
    "reset": {
    "type": "timeout_after_ms",
    "value": 5000
    },
    "take_picture_at_the_end": true
}
```

---

## Picture-Taking Options

You can trigger a photo at **gesture-end**, at **any step**, or at **only one random step**:

- **Gesture-level:**  
    Set `"take_picture_at_the_end": true` at the root.  
    → Photo will be triggered when the gesture completes.

- **Step-level:**  
    Add `"take_picture_at_the_end": true` in any instruction/step.  
    → Photo will be taken when that step is completed. Multiple steps can mark this.

- **Random step photo:**  
    At the gesture root, add `"randomize_step_picture": true`.  
    → If two or more instructions set `"take_picture_at_the_end": true`, only **one** randomly chosen such step will actually trigger the photo per gesture execution.

- **Both:**  
    You can set both gesture-level and step-level photo triggers. Both will fire.

- **No photo:**  
    Omit or set all flags to `false`.

---

## Examples

### Example 1: Both Gesture-End and Per-Step Photo

```json
{
    "gestureId": "smile",
    "label": "Smile wide",
    "icon_path": "",
    "signal_key": "mouthSmile",
    "total_recommended_max_time": 10000,
    "take_picture_at_the_end": true,   // triggers at gesture completion
    "randomize_step_picture": false,
    "instructions": [
    {
        "instruction_type": "threshold",
        "move_to_next_type": "higher",
        "value": 0.6,
        "reset": { "type": "timeout_after_ms", "value": 5000 },
        "take_picture_at_the_end": true  // triggers at this step
    },
    {
        "instruction_type": "threshold",
        "move_to_next_type": "lower",
        "value": 0.3,
        "reset": { "type": "timeout_after_ms", "value": 5000 }
    }
    ]
}
```

### Example 2: Only Randomized Step Picture

```json
{
    "gestureId": "wink",
    "label": "Wink",
    "icon_path": "",
    "signal_key": "eyeBlinkRight",
    "total_recommended_max_time": 9000,
    "take_picture_at_the_end": false,
    "randomize_step_picture": true,   // Only one of steps 0 or 2 fires for each gesture
    "instructions": [
    {
        "instruction_type": "threshold",
        "move_to_next_type": "lower",
        "value": 0.4,
        "reset": { "type": "timeout_after_ms", "value": 5000 },
        "take_picture_at_the_end": true
    },
    {
        "instruction_type": "range",
        "min_value": 0.1,
        "max_value": 0.2,
        "min_duration_ms": 1500,
        "reset": { "type": "timeout_after_ms", "value": 3000 }
    },
    {
        "instruction_type": "threshold",
        "move_to_next_type": "higher",
        "value": 0.45,
        "reset": { "type": "timeout_after_ms", "value": 5000 },
        "take_picture_at_the_end": true
    }
    ]
}
```

### Example 3: No Photo

```json
{
    "gestureId": "nod",
    "label": "Nod head",
    "icon_path": "",
    "signal_index": 4,
    "total_recommended_max_time": 7000,
    "take_picture_at_the_end": false,
    "instructions": [
    {
        "move_to_next_type": "higher",
        "value": 0.5,
        "reset": { "type": "timeout_after_ms", "value": 4000 }
    },
    {
        "move_to_next_type": "lower",
        "value": 0.2,
        "reset": { "type": "timeout_after_ms", "value": 4000 }
    }
    ]
}
```

---

## Tips & Best Practices

- Use only `"randomize_step_picture": true` if you want a **single random step** (among those marked per-step) to trigger photo, and not every marked step.
- You can combine gesture-level (`take_picture_at_the_end`) and step-level photo triggers.
- Don’t use both `signal_key` and `signal_index` in the same gesture file.
- Omit `"instruction_type"` or set it to `"threshold"` for traditional threshold steps.
- Use `"instruction_type": "range"` for range/hold steps.
- If you want no photo triggers, set both gesture and all step-level `take_picture_at_the_end` to `false` (or omit them).
- Always test your gestures to ensure detection and photo triggers behave as expected.

---

## Schema (Summary)

**Threshold step:**
```json
{
    "instruction_type": "threshold", // (optional)
    "move_to_next_type": "higher" | "lower",
    "value": <number>,
    "reset": {
    "type": "lower" | "higher" | "timeout_after_ms",
    "value": <number>
    },
    "take_picture_at_the_end": <boolean> // optional
}
```

**Range/Hold step:**
```json
{
    "instruction_type": "range",
    "min_value": <number>,
    "max_value": <number>,
    "min_duration_ms": <integer>,
    "reset": {
    "type": "lower" | "higher" | "timeout_after_ms",
    "value": <number>
    },
    "take_picture_at_the_end": <boolean> // optional
}
```

**Gesture file:**
```json
{
    "gestureId": "...",
    "label": "...",
    "icon_path": "...",
    "signal_key": "..." // or "signal_index": ...,
    "total_recommended_max_time": 10000,
    "take_picture_at_the_end": false,
    "randomize_step_picture": false,  // optional, default false
    "instructions": [ { ... }, ... ]
}
```

---

### Callback/event summary

- **Gesture-level** (`take_picture_at_the_end` root):  
    Photo taken at gesture completion.
- **Step-level** (`take_picture_at_the_end` in instructions):  
    Photo taken after that step (unless `randomize_step_picture` is set, then only one marked step triggers, chosen randomly).
- **Both can be used together**.  
- **Omitting both**: No photo events for that gesture.

---