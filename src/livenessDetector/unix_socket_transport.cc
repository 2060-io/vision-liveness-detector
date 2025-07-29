#include "unix_socket_transport.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cerrno>

UnixSocketTransport::UnixSocketTransport(const std::string& socket_path)
    : m_socket_path(socket_path), m_fd(-1), m_is_server(false), m_bound(false)
{}

UnixSocketTransport::~UnixSocketTransport() {
    close();
}

void UnixSocketTransport::close() {
    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_is_server && m_bound) {
        unlink(m_socket_path.c_str());
        m_bound = false;
    }
}

bool UnixSocketTransport::open_server() {
    m_is_server = true;
    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd == -1) {
        perror("socket");
        return false;
    }
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);
    unlink(m_socket_path.c_str());
    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("bind");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    m_bound = true;
    if (::listen(m_fd, 5) == -1) {
        perror("listen");
        ::close(m_fd);
        m_fd = -1;
        m_bound = false;
        return false;
    }
    return true;
}

UnixSocketTransport* UnixSocketTransport::accept_client() {
    if (m_fd == -1) return nullptr;
    int client_fd = ::accept(m_fd, nullptr, nullptr);
    if (client_fd == -1) {
        perror("accept");
        return nullptr;
    }
    return new UnixSocketTransport(client_fd, m_socket_path);
}

UnixSocketTransport::UnixSocketTransport(int fd, const std::string& socket_path)
    : m_socket_path(socket_path), m_fd(fd), m_is_server(false), m_bound(false)
{}

bool UnixSocketTransport::open_client() {
    m_is_server = false;
    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd == -1) {
        perror("client socket");
        return false;
    }
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, m_socket_path.c_str(), sizeof(addr.sun_path) - 1);
    if (::connect(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("client connect");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    return true;
}

bool UnixSocketTransport::read(void* buffer, size_t size) {
    size_t total = 0;
    uint8_t* buf = static_cast<uint8_t*>(buffer);
    while (total < size) {
        ssize_t r = ::read(m_fd, buf + total, size - total);
        if (r <= 0) {
            return false;
        }
        total += static_cast<size_t>(r);
    }
    return true;
}

bool UnixSocketTransport::write(const void* buffer, size_t size) {
    size_t total = 0;
    const uint8_t* buf = static_cast<const uint8_t*>(buffer);
    while (total < size) {
        ssize_t w = ::write(m_fd, buf + total, size - total);
        if (w <= 0) {
            return false;
        }
        total += static_cast<size_t>(w);
    }
    return true;
}

bool UnixSocketTransport::is_open() const {
    return m_fd != -1;
}

int UnixSocketTransport::get_fd() const {
    return m_fd;
}