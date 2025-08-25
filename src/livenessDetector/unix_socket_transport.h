#pragma once

#include "transport.h"
#include <string>
#include <memory>

// Unix domain socket implementation of the Transport interface.
class UnixSocketTransport : public Transport {
public:
    explicit UnixSocketTransport(const std::string& socket_path);
    UnixSocketTransport(UnixSocketTransport&&) noexcept;
    UnixSocketTransport& operator=(UnixSocketTransport&&) noexcept;
    ~UnixSocketTransport() override;

    // Server methods
    bool open_server() override;
    std::unique_ptr<UnixSocketTransport> accept_client(); // only used by servers

    // Client methods
    bool open_client() override;

    void close() noexcept override;

    bool is_open() const override;
    int get_fd() const override;

    bool read_exact(void* buffer, size_t size) override;
    bool write_exact(const void* buffer, size_t size) override;

    // Expose this for testability
    const std::string& socket_path() const { return m_socket_path; }

private:
    std::string m_socket_path;
    int m_fd;
    bool m_is_server;
    bool m_owns_socket_path;

    // Internal use for accepted sockets
    UnixSocketTransport(int fd, const std::string& socket_path);

    UnixSocketTransport(const UnixSocketTransport&) = delete;
    UnixSocketTransport& operator=(const UnixSocketTransport&) = delete;
};