#pragma once

#include <cstddef>

// Abstract transport interface for generic binary connections.
class Transport {
public:
    virtual ~Transport() {}

    // For server side: bind/listen. For client: connect.
    virtual bool open_server() = 0;
    virtual bool open_client() = 0;
    virtual void close() = 0;

    virtual bool is_open() const = 0;
    virtual int  get_fd()  const = 0;

    virtual bool read(void* buffer, size_t size) = 0;           // blocking, exact size, full buffer (for now)
    virtual bool write(const void* buffer, size_t size) = 0;    // blocking, exact size, full buffer (for now)
};