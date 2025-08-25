#include "unix_socket_transport.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <cstdio>
#include <cassert>
#include <sys/stat.h>
#include <memory>
#include <iostream>
#include <iomanip>


UnixSocketTransport::UnixSocketTransport(const std::string& socket_path)
    : m_socket_path(socket_path)
    , m_fd(-1)
    , m_is_server(false)
    , m_owns_socket_path(false)
{}

UnixSocketTransport::UnixSocketTransport(int fd, const std::string& socket_path)
    : m_socket_path(socket_path)
    , m_fd(fd)
    , m_is_server(false)
    , m_owns_socket_path(false)
{}

UnixSocketTransport::UnixSocketTransport(UnixSocketTransport&& other) noexcept
    : m_socket_path(std::move(other.m_socket_path))
    , m_fd(other.m_fd)
    , m_is_server(other.m_is_server)
    , m_owns_socket_path(other.m_owns_socket_path)
{
    other.m_fd = -1;
    other.m_owns_socket_path = false;
}
UnixSocketTransport& UnixSocketTransport::operator=(UnixSocketTransport&& other) noexcept {
    if (this != &other) {
        close();
        m_socket_path = std::move(other.m_socket_path);
        m_fd = other.m_fd;
        m_is_server = other.m_is_server;
        m_owns_socket_path = other.m_owns_socket_path;
        other.m_fd = -1;
        other.m_owns_socket_path = false;
    }
    return *this;
}

UnixSocketTransport::~UnixSocketTransport() {
    close();
}

void UnixSocketTransport::close() noexcept {
    if (m_fd != -1) {
        ::shutdown(m_fd, SHUT_RDWR);  // Ignore result (may not be connected)
        ::close(m_fd);
        m_fd = -1;
    }
    if (m_is_server && m_owns_socket_path && !m_socket_path.empty()) {
        // Remove socket file only if we created it.
        ::unlink(m_socket_path.c_str());
        m_owns_socket_path = false;
    }
}

bool UnixSocketTransport::open_server() {
    close();
    m_is_server = true;
    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd == -1) {
        perror("socket");
        return false;
    }
    // Construct address
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;

    // Make sure we never overflow sun_path.
    size_t max_path = sizeof(addr.sun_path) - 1;
    if (m_socket_path.length() > max_path) {
        fprintf(stderr, "Socket path too long\n");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    std::strncpy(addr.sun_path, m_socket_path.c_str(), max_path);
    addr.sun_path[max_path] = '\0';
    // Remove any existing socket on path (this is susceptible to TOCTOU; for mitigating, check file type in prod)
    struct stat st;
    if (::lstat(m_socket_path.c_str(), &st) == 0) {
        if (S_ISSOCK(st.st_mode)) {
            ::unlink(m_socket_path.c_str());
        } else {
            fprintf(stderr, "Socket path %s exists and is not a socket\n", m_socket_path.c_str());
            ::close(m_fd);
            m_fd = -1;
            return false;
        }
    }

    // Use correct length: offsetof + path len + 1
    size_t addrlen = offsetof(sockaddr_un, sun_path) + std::strlen(addr.sun_path) + 1;
    if (::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), addrlen) == -1) {
        perror("bind");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    m_owns_socket_path = true;
    // Set permissions
    ::chmod(m_socket_path.c_str(), 0700); // owner only

    if (::listen(m_fd, 16) == -1) {
        perror("listen");
        this->close();
        return false;
    }
    return true;
}

std::unique_ptr<UnixSocketTransport> UnixSocketTransport::accept_client() {
    if (m_fd == -1) return nullptr;

    int client_fd;
    ssize_t result;
    do {
        client_fd = ::accept(m_fd, nullptr, nullptr);
    } while (client_fd == -1 && errno == EINTR);

    if (client_fd == -1) {
        perror("accept");
        return nullptr;
    }
    // TODO: In production consider SO_PEERCRED or getpeereid() to verify client credentials!
    return std::unique_ptr<UnixSocketTransport>(new UnixSocketTransport(client_fd, m_socket_path));
}

bool UnixSocketTransport::open_client() {
    close();
    m_is_server = false;
    m_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_fd == -1) {
        perror("client socket");
        return false;
    }
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    size_t max_path = sizeof(addr.sun_path) - 1;
    if (m_socket_path.length() > max_path) {
        fprintf(stderr, "Client socket path too long\n");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    std::strncpy(addr.sun_path, m_socket_path.c_str(), max_path);
    addr.sun_path[max_path] = '\0';
    size_t addrlen = offsetof(sockaddr_un, sun_path) + std::strlen(addr.sun_path) + 1;

    // Retry connect on EINTR.
    int res;
    do {
        res = ::connect(m_fd, reinterpret_cast<sockaddr*>(&addr), addrlen);
    } while (res == -1 && errno == EINTR);

    if (res == -1) {
        perror("client connect");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }
    return true;
}

// Handles partial, EINTR, and 0 (EOF) correctly.
bool UnixSocketTransport::read_exact(void* buffer, size_t size) {
    size_t total = 0;
    uint8_t* buf = static_cast<uint8_t*>(buffer);
    while (total < size) {
        ssize_t r;
        do {
            r = ::read(m_fd, buf + total, size - total);
        } while (r == -1 && errno == EINTR);
        if (r == 0) {
            // EOF
            return false;
        }
        if (r < 0) {
            return false;
        }
        total += static_cast<size_t>(r);
    }
    return true;
}

bool UnixSocketTransport::write_exact(const void* buffer, size_t size) {
    size_t total = 0;
    const uint8_t* buf = static_cast<const uint8_t*>(buffer);
    while (total < size) {
        ssize_t w;
#if defined(MSG_NOSIGNAL)
        do {
            w = ::send(m_fd, buf + total, size - total, MSG_NOSIGNAL);
        } while (w == -1 && errno == EINTR);
#else
        do {
            w = ::write(m_fd, buf + total, size - total);
        } while (w == -1 && errno == EINTR);
#endif
        if (w <= 0) {
            std::cerr << "[write_exact] write error: "
                      << strerror(errno) << " (errno=" << errno << "), "
                      << "fd=" << m_fd << ", after writing " << total << " of " << size << " bytes.\n";
            return false;
        }
        total += static_cast<size_t>(w);
    }
    //std::cerr << "[write_exact] Success: wrote " << size << " bytes to fd=" << m_fd;
    //
    //// Print first 4 bytes (or less), as hex
    //size_t print_bytes = std::min(size_t(4), size);
    //const uint8_t* b = static_cast<const uint8_t*>(buffer);
    //std::cerr << " [first " << print_bytes << " bytes: ";
    //for (size_t i = 0; i < print_bytes; ++i) {
    //    std::cerr << std::hex << std::setw(2) << std::setfill('0') << (unsigned)b[i] << " ";
    //}
    //std::cerr << "]\n";

    return true;
}

bool UnixSocketTransport::is_open() const {
    return m_fd != -1;
}

int UnixSocketTransport::get_fd() const {
    return m_fd;
}