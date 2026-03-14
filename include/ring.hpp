#pragma once

#include "cppcoro/task.hpp"

#include "buffer.hpp"

#include <coroutine>
#include <cstddef>
#include <memory>
#include <vector>

struct io_uring;

namespace cororing {

bool is_io_uring_supported();
std::vector<const char*> get_capabilities();

class ring_t {
public:
    ring_t(size_t queue_size);
    ~ring_t();

    ring_t(const ring_t& other)            = delete;
    ring_t& operator=(const ring_t& other) = delete;

    ring_t(ring_t&& other)            = default;
    ring_t& operator=(ring_t&& other) = default;

    bool poll();

    cppcoro::task<int> read(buffer_t buffer, int client_socket);
    cppcoro::task<int> read_until_full(buffer_t buffer, int client_socket);
    cppcoro::task<int> write(buffer_t buffer, int client_socket);
    cppcoro::task<int> accept(int listen_socket);
    cppcoro::task<int> close(int client_socket);

private:
    struct request_t;

    request_t* create_request();
    void delete_request(request_t* request);

    void process();

    void read(buffer_t buffer, int client_socket, std::coroutine_handle<> coro, int* result);
    void write(buffer_t buffer, int client_socket, std::coroutine_handle<> coro, int* result);
    void accept(int listen_socket, std::coroutine_handle<> coro, int* result);
    void close(int client_socket, std::coroutine_handle<> coro, int* result);

    std::unique_ptr<struct io_uring> ring_ { nullptr };
    size_t queue_size_ {};
    size_t submissions_ {};
};

}
