#pragma once

#include "cppcoro/task.hpp"

#include <coroutine>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

struct io_uring;

namespace cororing {

using buffer_t = std::span<std::byte>;

std::vector<std::string> get_capabilities();

class iouring {
public:
    iouring(size_t queue_size);
    ~iouring();

    bool poll();

    cppcoro::task<int> read(buffer_t buffer, int client_socket);
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
