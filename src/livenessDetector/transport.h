#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

// Abstract transport interface for generic binary connections.
class Transport {
public:
    virtual ~Transport() = default;

    // For server side: bind/listen. For client: connect.
    virtual bool open_server() = 0;
    virtual bool open_client() = 0;
    virtual void close() noexcept = 0;

    virtual bool is_open() const = 0;
    virtual int  get_fd()  const = 0;

    // Blocking, reads exactly 'size' bytes, returns false on error/EOF. Buffer must be at least size bytes.
    virtual bool read_exact(void* buffer, size_t size) = 0;
    // Blocking, writes exactly 'size' bytes, returns false on error/EOF.
    virtual bool write_exact(const void* buffer, size_t size) = 0;
};