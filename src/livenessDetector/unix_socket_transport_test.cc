#include "unix_socket_transport.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>

// Helper function for creating a unique temporary path
std::string make_temp_path() {
    char templ[] = "/tmp/unix_socket_testXXXXXX";
    int fd = mkstemp(templ);
    if (fd != -1) {
        ::close(fd);
        ::unlink(templ);
    }
    return templ;
}

TEST(UnixSocketTransportTest, ServerClientCommunication) {
    auto path = make_temp_path();
    UnixSocketTransport server(path);
    ASSERT_TRUE(server.open_server());
    std::atomic<bool> client_done{false};
    std::string send_msg = "Hello, world!";
    std::string recv_msg(send_msg.size(), '\0');
    std::string reply = "Hello client!";
    std::string reply_buf(reply.size(), '\0');

    // Start server accept in another thread
    std::thread server_thread([&](){
        auto conn = server.accept_client();
        ASSERT_TRUE(conn != nullptr);
        ASSERT_TRUE(conn->read_exact(&recv_msg[0], recv_msg.size()));
        ASSERT_EQ(recv_msg, send_msg);
        ASSERT_TRUE(conn->write_exact(reply.data(), reply.size()));
        client_done = true;
    });

    // Client
    UnixSocketTransport client(path);
    ASSERT_TRUE(client.open_client());
    ASSERT_TRUE(client.write_exact(send_msg.data(), send_msg.size()));
    ASSERT_TRUE(client.read_exact(&reply_buf[0], reply_buf.size()));
    ASSERT_EQ(reply_buf, reply);

    // Wait for server
    for (int i = 0; i < 10 && !client_done; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(client_done);

    server_thread.join();
    server.close();
    client.close();
}

TEST(UnixSocketTransportTest, PartialReadsAndWrites) {
    auto path = make_temp_path();
    UnixSocketTransport server(path);
    ASSERT_TRUE(server.open_server());
    std::atomic<bool> client_done{false};

    std::vector<uint8_t> payload(1024*100, 42); // 100 KiB
    std::vector<uint8_t> buffer(payload.size(), 0);

    std::thread server_thread([&]() {
        auto conn = server.accept_client();
        ASSERT_TRUE(conn != nullptr);
        ASSERT_TRUE(conn->read_exact(buffer.data(), buffer.size()));
        ASSERT_EQ(buffer, payload);
        client_done = true;
    });

    UnixSocketTransport client(path);
    ASSERT_TRUE(client.open_client());
    ASSERT_TRUE(client.write_exact(payload.data(), payload.size()));

    for (int i = 0; i < 10 && !client_done; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_TRUE(client_done);

    server_thread.join();
}

TEST(UnixSocketTransportTest, DoubleCloseDoesNotCrash) {
    auto path = make_temp_path();
    UnixSocketTransport server(path);
    ASSERT_TRUE(server.open_server());
    server.close();
    server.close();
    // Should not crash nor double-unlink.
}

TEST(UnixSocketTransportTest, TooLongPathFails) {
    std::string longpath(300, 'a');
    UnixSocketTransport server(longpath);
    ASSERT_FALSE(server.open_server());
}

TEST(UnixSocketTransportTest, SocketFileIsRemoved) {
    auto path = make_temp_path();
    {
        UnixSocketTransport server(path);
        ASSERT_TRUE(server.open_server());
        struct stat st;
        ASSERT_EQ(::lstat(path.c_str(), &st), 0);
        ASSERT_TRUE(S_ISSOCK(st.st_mode));
    }
    // socket file should be gone
    struct stat st;
    ASSERT_NE(::lstat(path.c_str(), &st), 0);
}