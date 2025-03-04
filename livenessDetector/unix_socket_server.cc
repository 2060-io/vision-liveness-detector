#include "unix_socket_server.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>

UnixSocketServer::UnixSocketServer(const std::string& socketPath, ImageProcessingCallback callback)
    : socketPath(socketPath), processImageCallback(std::move(callback)), server_fd(-1) {}

UnixSocketServer::~UnixSocketServer() {
    if (server_fd != -1) {
        close(server_fd);
        unlink(socketPath.c_str());
    }
}

bool UnixSocketServer::start() {
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return false;
    }

    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socketPath.c_str(), sizeof(server_addr.sun_path) - 1);

    unlink(socketPath.c_str()); // Remove previous socket file

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        server_fd = -1;
        return false;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        server_fd = -1;
        return false;
    }

    std::cout << "Server started, listening for connections...\n";
    return true;
}

void UnixSocketServer::run() {
    if (server_fd == -1) {
        std::cerr << "Server socket not initialized\n";
        return;
    }

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        std::cout << "Client connected...\n";

        while (processClient(client_fd)) {}

        std::cout << "Client disconnected...\n";
        close(client_fd);
    }
}

bool UnixSocketServer::processClient(int client_fd) {
    uint8_t function_id;
    ssize_t bytes_read = read(client_fd, &function_id, sizeof(function_id));
    if (bytes_read != sizeof(function_id)) {
        std::cerr << "Failed to read function ID\n";
        return false;
    }

    if (function_id == 0x01) { // Image processing
        uint32_t frame_size;
        bytes_read = read(client_fd, &frame_size, sizeof(frame_size));
        if (bytes_read != sizeof(frame_size)) {
            std::cerr << "Failed to read frame size\n";
            return false;
        }
        frame_size = ntohl(frame_size);

        uint32_t rows, cols;
        if (read(client_fd, &rows, sizeof(rows)) != sizeof(rows)) return false;
        if (read(client_fd, &cols, sizeof(cols)) != sizeof(cols)) return false;
        rows = ntohl(rows);
        cols = ntohl(cols);

        std::vector<uchar> frame_data(frame_size);
        size_t total_bytes_received = 0;
        while (total_bytes_received < frame_size) {
            ssize_t bytes_received = read(client_fd, frame_data.data() + total_bytes_received, frame_size - total_bytes_received);
            if (bytes_received <= 0) {
                std::cerr << "Client disconnected or read error\n";
                return false;
            }
            total_bytes_received += bytes_received;
        }

        cv::Mat img(rows, cols, CV_8UC3, frame_data.data());
        auto [processed_img, info_string] = processImageCallback(img);

        if (!info_string.empty()) {
            // Send string information
            uint8_t response_function_id = 0x02; // For string data
            uint32_t string_size = info_string.size();
            string_size = htonl(string_size);

            write(client_fd, &response_function_id, sizeof(response_function_id));
            write(client_fd, &string_size, sizeof(string_size));
            write(client_fd, info_string.data(), info_string.size());
        }

        // Always send the processed image
        uint8_t response_function_id = 0x01; // For image processing
        write(client_fd, &response_function_id, sizeof(response_function_id));

        uint32_t processed_size = htonl(processed_img.total() * processed_img.elemSize());
        uint32_t net_rows = htonl(processed_img.rows);
        uint32_t net_cols = htonl(processed_img.cols);

        write(client_fd, &processed_size, sizeof(processed_size));
        write(client_fd, &net_rows, sizeof(net_rows));
        write(client_fd, &net_cols, sizeof(net_cols));
        write(client_fd, processed_img.data, processed_img.total() * processed_img.elemSize());
    } else if (function_id == 0x02) {
        // Handle another function
    }

    return true;
}