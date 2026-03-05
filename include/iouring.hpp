#pragma once

#include "function.hpp"

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

    void poll();

    void read(buffer_t buffer, int client_socket, co::move_only_function<void, int> callback);
    void write(buffer_t buffer, int client_socket, co::move_only_function<void, int> callback);
    void accept(int listen_socket, co::move_only_function<void, int> callback);

    class awaiter {
    public:
        awaiter(co::move_only_function<void, co::move_only_function<void, int>> callback)
            : callback_(std::move(callback))
        {
        }

        bool await_ready()
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            callback_([this, handle](int result) {
                value_ = result;
                handle.resume();
            });
        }

        int await_resume()
        {
            return value_;
        }

    private:
        int value_ {};
        co::move_only_function<void, co::move_only_function<void, int>> callback_ {};
    };

    awaiter read(buffer_t buffer, int client_socket)
    {
        return awaiter { [this, buffer, client_socket](co::move_only_function<void, int> callback) mutable {
            read(buffer, client_socket, std::move(callback));
        } };
    }

    awaiter write(buffer_t buffer, int client_socket)
    {
        return awaiter { [this, buffer, client_socket](co::move_only_function<void, int> callback) mutable {
            write(buffer, client_socket, std::move(callback));
        } };
    }

    awaiter accept(int listen_socket)
    {
        return awaiter { [this, listen_socket](co::move_only_function<void, int> callback) mutable {
            accept(listen_socket, std::move(callback));
        } };
    }

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
