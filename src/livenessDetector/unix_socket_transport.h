#pragma once

#include "transport.h"
#include <string>

// Unix domain socket implementation of the Transport interface.
class UnixSocketTransport : public Transport {
public:
    explicit UnixSocketTransport(const std::string& socket_path);
    ~UnixSocketTransport() override;

    // Server methods
    bool open_server() override;
    UnixSocketTransport* accept_client(); // only used by servers

    // Client methods
    bool open_client() override;

    void close() override;

    bool is_open() const override;
    int get_fd() const override;

    bool read(void* buffer, size_t size) override;
    bool write(const void* buffer, size_t size) override;

private:
    std::string m_socket_path;
    int m_fd;
    bool m_is_server;
    bool m_bound;

    // Internal use for accepted sockets
    UnixSocketTransport(int fd, const std::string& socket_path);

    UnixSocketTransport(const UnixSocketTransport&) = delete;
    UnixSocketTransport& operator=(const UnixSocketTransport&) = delete;
};