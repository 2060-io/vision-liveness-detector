# General Purpose Message Framing Protocol — Specification

This document describes a general-purpose, binary message framing protocol that enables efficient, extensible and robust communication between independent applications or components. The protocol is transport-agnostic and supports sending and receiving typed messages (such as images, JSON, or combined messages) over any reliable, in-order byte stream (for example, Unix domain socket, TCP, pipes).

---

## Table of Contents

1. [Overview](#overview)
2. [Transport Layer Abstraction](#transport-layer-abstraction)
3. [Message Framing and Structure](#message-framing-and-structure)
    - [Message Types](#message-types)
    - [0x01: Image Message](#0x01-image-message)
    - [0x02: JSON Message](#0x02-json-message)
    - [0x03: Combined Images + JSON Message](#0x03-combined-images--json-message)
4. [Protocol Handler Responsibilities](#protocol-handler-responsibilities)
5. [Expected Responses / Return Values](#expected-responses--return-values)
6. [Example Communication Flows](#example-communication-flows)
7. [Integration Guidelines (C++/Python)](#integration-guidelines-cpython)
8. [Extending the Protocol](#extending-the-protocol)
9. [Error Handling and Security](#error-handling-and-security)
10. [Wire Format Reference](#wire-format-reference)
11. [Summary](#summary)

---

## Overview

This protocol provides a simple, robust, and extensible method for the exchange of binary or structured data between programs. Key features:

- **Binary framing**: Each message has a type and an explicit (size-prefixed) payload.
- **Typed & Extensible**: Adding new message types does not affect old implementations (unknown types can be skipped/rejected).
- **Efficient**: Images and bulky data are sent raw for maximum throughput when needed.
- **Bidirectional**: Any endpoint can send or receive messages.
- **Transport-agnostic**: Use over any reliable byte stream.

**Current default transport:** Unix domain socket.  
**But:** You can use any stream transport (TCP, pipe, etc.) by implementing the transport interface.

---

## Transport Layer Abstraction

The protocol **requires a reliable, in-order, lossless stream transport**. Implementation provides a `Transport` interface (C++: `read_exact`, `write_exact`; Python: `sendall`, `recv`) that is responsible for reading and writing the exact number of bytes requested.

- **C++:** You can substitute `UnixSocketTransport` for your own TCP, named pipe, etc.
- **Python:** Similarly, by implementing the `Transport` interface.

No protocol logic assumes a particular transport; framing handles arbitrary message lengths and boundaries.

---

## Message Framing and Structure

All messages are binary framed:

```plaintext
+----------+----------------------+
| 1-byte   | Variable Payload     |
| Type     | (per type)           |
+----------+----------------------+
```

### Message Types

| Value    | Description                           |
|----------|---------------------------------------|
| `0x01`   | Image Message (single image)          |
| `0x02`   | JSON Message (string/structured data) |
| `0x03`   | Combined Images + JSON Message        |

Unknown types are handled gracefully (rejected or ignored according to implementation policy).

---

### 0x01: Image Message

Transfers a single image.

**Wire Format:**
```
+------+----------+----------+----------+--------------+
|0x01  | size[4]  | rows[4]  | cols[4]  | data[size]   |
+------+----------+----------+----------+--------------+
```
All fields in big-endian byte order.

| Field    | Bytes | Description                      |
|----------|-------|----------------------------------|
| type     | 1     | 0x01                             |
| size     | 4     | N = rows * cols * channels       |
| rows     | 4     | Image height                     |
| cols     | 4     | Image width                      |
| data     | N     | Image bytes (e.g., BGR, uint8)   |

- Data layout: row-major pixels, usually 3 channels BGR (`uint8`) per pixel.
- Max payload size: **16 MiB**.

---

### 0x02: JSON Message

JSON, UTF-8 encoded string.

**Wire Format:**
```
+------+----------+-------------------+
|0x02  | size[4]  | data[size]        |
+------+----------+-------------------+
```

| Field    | Bytes | Description                   |
|----------|-------|-------------------------------|
| type     | 1     | 0x02                          |
| size     | 4     | Length of JSON data N         |
| data     | N     | UTF-8 JSON string             |

- Max payload size: **1 MiB**.

---

### 0x03: Combined Images + JSON Message

Allows you to send any number of images (each with an ID), plus a JSON string—all in one atomic message.  
Each image is identified by a user-supplied uint32 ID and includes its own pixel data.

**Wire Format:**
```
+------+------------+----------------------+---------------+--------------+
|0x03  | num_imgs[4]| [img blocks]*N       | json_size[4]  | json_bytes   |
+------+------------+----------------------+---------------+--------------+

Each [img block]:
+---------+----------+----------+----------+------------+
|img_id[4]|size[4]   |rows[4]   |cols[4]   |data[size]  |
+---------+----------+----------+----------+------------+
```

| Field         | Bytes | Description                                      |
|---------------|-------|--------------------------------------------------|
| type          | 1     | 0x03                                             |
| num_imgs      | 4     | Number of images (N)                             |
| img blocks    | *     | Each block: img_id, size, rows, cols, data[]     |
| json_size     | 4     | Size (in bytes) of JSON string                   |
| json_bytes    | n     | UTF-8 JSON string                                |

- Each image block:
    - `img_id` (uint32): Application-level identifier for the image
    - `size` (uint32): Size of image bytes (should equal rows*cols*channels for 8-bit BGR)
    - `rows/cols`: dimensions
    - `data[size]`: Image pixel bytes, row-major
- JSON string may reference images by ID, or be unrelated.
- Max images per message: **32** (or protocol constant set by sender/receiver).
- Max per-image size: **16 MiB**
- Max JSON size: **1 MiB**

---

## Protocol Handler Responsibilities

ProtocolHandler is the layer responsible for:
- Reading/writing *framed* messages with type and size fields.
- Enforcing size and type constraints.
- Providing application callbacks (per message type) for handling the data.
- Supporting sending *any* of the message types as needed.

In both C++ and Python, ProtocolHandler does not own the transport, but depends on it for stream reads/writes.

---

## Expected Responses / Return Values

The protocol **does not mandate a specific required response** for any command. Each endpoint (client/server/peer) can reply with any set of messages, or not reply at all, according to the application’s needs. The handler can respond to any received message type using any of the message types.

### 0x01: Image Message

- **Typical Response:**  
    - One or more 0x02 (JSON) messages (e.g., status, event, metadata, errors)
    - Optionally, one or more 0x01 (image) messages (e.g., processed/annotated images)
    - Or, no response at all

- **Example:**  
    - Image-in → JSON-out (optional) → Image-out (optional)

### 0x02: JSON Message

- **Typical Response:**  
    - May result in a 0x02 (JSON) message (acknowledgement, error, status) or no response
- **Example:**  
    - JSON-in → JSON-out (optional ack or error)

### 0x03: Combined Images + JSON Message

- **Typical Response:**  
    - May result in one or more 0x02 (JSON) messages (status), a 0x01 or 0x03 (image(s)), or no response
- **Example:**  
    - Combined-in → JSON-out (optional) → Combined-out or Image-out (optional)

#### **Summary Table**

| Command Type | Typical (but not mandatory) Response(s)                                  | Notes                                       |
|--------------|--------------------------------------------------------------------------|---------------------------------------------|
| 0x01 Image   | JSON (status/event/error), Image (processed), or nothing                 | Respond with 0x01 or 0x02 as desired        |
| 0x02 JSON    | JSON (ack, status, error), or nothing                                    | 0x02 for ack/errors or silently process     |
| 0x03 Combined| JSON (status), Combined (result), or nothing                             | Respond with any type or nothing            |

- Multiple responses are allowed for a single incoming message.
- The pattern is ultimately determined by your application logic and use case.
- Handlers must correctly deal with unexpected or absent responses.

---

## Example Communication Flows

### Send/Receive a Single Image

1. Sender packages image as type 0x01 frame, sends it.
2. Receiver parses, reconstructs image.
3. Receiver can reply with type 0x02 (JSON response), 0x01 (image), or ignore.

### Send/Receive JSON Config

1. Sender sends UTF-8 JSON string as type 0x02 message.
2. Receiver parses JSON, takes config action.
3. Any endpoint can send more messages in any order (duplex).

### Send/Receive Combined Images + JSON

1. Sender packages N images with IDs, then a JSON blob.
2. Receiver gets all images and the JSON atomically, in a single callback.
3. Useful for cases where structured data and binary images must be sent together, or in batch.

---    

## Integration Guidelines (C++/Python)

### C++

- Implement your handlers for each ProtocolMessageType. Pass in callbacks for image, JSON, and combined message.
- Use `send_image`, `send_json`, and `send_combined` to transmit messages.
- On Combined, callback receives a list of `(img_id, cv::Mat)` and a JSON string.

### Python

- Use `send_image`, `send_json`, `send_combined`.
- When `recv_message()` returns `fid == 0x03`, will receive a tuple: `(images, json_str)` where `images` is a list of `(img_id, np.ndarray)`.

---

## Extending the Protocol

- New ProtocolMessageType values (e.g., 0x04) can be added safely; old handlers will reject or skip unknown types.
- Always use explicit size-fields, and keep wire format unambiguous.
- Always update documentation and tests with each extension.

---

## Error Handling and Security

- **Size Constraints:** Always check per-message/image payload sizes. Reject oversize messages.
- **Unknown Types:** ProtocolHandler should either log and skip, or terminate session.
- **Malformed Input:** On incomplete or malformed messages, connection/session should be terminated safely.
- **Transport Security:** If using TCP/IP, consider adding an authenticated transport (TLS, etc).

---

## Wire Format Reference

### 0x01: Image Message

| Offset | Bytes | Field        | Description                 |
|--------|-------|--------------|-----------------------------|
| 0      | 1     | fid          | 0x01 (Image Message)        |
| 1      | 4     | size         | Payload size, big-endian    |
| 5      | 4     | rows         | Image height, big-endian    |
| 9      | 4     | cols         | Image width, big-endian     |
| 13     | N     | data         | Pixel data, N = size        |

### 0x02: JSON Message

| Offset | Bytes | Field        | Description                 |
|--------|-------|--------------|-----------------------------|
| 0      | 1     | fid          | 0x02 (JSON Message)         |
| 1      | 4     | size         | Payload size, big-endian    |
| 5      | N     | payload      | UTF-8 JSON                  |

### 0x03: Combined Images + JSON Message

| Offset        | Bytes        | Field         | Description                                       |
|---------------|--------------|---------------|---------------------------------------------------|
| 0             | 1            | fid           | 0x03 (Combined Message)                           |
| 1             | 4            | num_imgs      | Number of images (N, big-endian)                  |
| 5             | variable     | img blocks    | For each image: [img_id|size|rows|cols|data[]]    |
| ...           | 4            | json_size     | Size of JSON payload                              |
| ...           | N            | json payload  | JSON (UTF-8)                                      |

Each `[img block]`:

| Offset        | Bytes        | Field         | Description              |
|---------------|--------------|---------------|--------------------------|
| +0            | 4            | img_id        | Image identifier (u32)   |
| +4            | 4            | size          | Image size in bytes      |
| +8            | 4            | rows          | Image rows (height)      |
| +12           | 4            | cols          | Image cols (width)       |
| +16           | size         | data          | Raw image data           |

---

## Summary

- **General Purpose**: Binary, strongly framed, extensible protocol for images, JSON, and composite messages.
- **Transport-Agnostic**: Use over Unix/TCP/pipes as needed.
- **Extensible**: Add more message types as your use case grows.
- **Reference Implementations**: Provided for C++ and Python; see `protocol_handler.h/.cc` and `protocol_handler.py`.

See this document and the codebase for up-to-date wire format and usage patterns.