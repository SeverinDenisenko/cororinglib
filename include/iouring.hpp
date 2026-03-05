#pragma once

#include "function.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

struct io_uring;

namespace cororing {

using buffer = std::span<std::byte>;

std::vector<std::string> get_capabilities();

class iouring {
public:
    iouring(size_t queue_size);
    ~iouring();

    void poll();

    void read(buffer buffer, int client_socket, co::move_only_function<void, int> callback);

    void write(buffer buffer, int client_socket, co::move_only_function<void, int> callback);

    void accept(int listen_socket, co::move_only_function<void, int> callback);

private:
    struct request_t;

    request_t* create_request();
    void delete_request(request_t* request);
    void process();

    std::unique_ptr<struct io_uring> ring_ { nullptr };
    size_t queue_size_ {};
    size_t submissions_ {};
};

}
