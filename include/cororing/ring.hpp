#pragma once

#include "cppcoro/task.hpp"

#include "cororing/buffer.hpp"

#include <coroutine>
#include <cstddef>
#include <memory>
#include <vector>

/** @file */

struct io_uring;

namespace cororing {

/**
 * @brief
 *
 * @return true if io_uring supported and enabled on current platform
 * @return false if error was encounteded diring looking for io_uring
 */
bool is_io_uring_supported();

/**
 * @brief Get the io_uring capabilities on the current platform
 *
 * @return std::vector<const char*> Vector of named capabilities. If error was encounteded during lookup, returns empty
 * vector.
 */
std::vector<const char*> get_capabilities();

/**
 * @brief io_uring coroutine wrapper
 */
class ring_t {
public:
    /**
     * @brief Construct a new ring object
     *
     * @param queue_size Length of sumbition and completion queues. Must be bigger then 0.
     * @param queue_min_space_left Minimum amount of submition queue free space while processing completion queue.
     * Default is 0 (completion queue will be drainded until submition queue will be completly full). Must be smaller
     * then queue_size.
     */
    ring_t(size_t queue_size, size_t queue_min_space_left = 0);
    ~ring_t();

    ring_t(const ring_t& other)            = delete;
    ring_t& operator=(const ring_t& other) = delete;

    ring_t(ring_t&& other)            = default;
    ring_t& operator=(ring_t&& other) = default;

    /**
     * @brief Read events from completion queue and resume all coroutines awaiting responces
     *
     * @return true if completion queue had events
     * @return false if completion queue was empty
     */
    bool poll();

    /**
     * @brief Read from file descriptor
     *
     * @param buffer Non-owning reference to data
     * @param fd Valid file descriptor
     * @return cppcoro::task<int> Task yelding amount of bytes read if > 0 or error code if < 0
     */
    cppcoro::task<int> read(buffer_t buffer, int fd);

    /**
     * @brief Read exacly buffer.size() bytes from file descriptor
     *
     * @param buffer Non-owning reference to data
     * @param fd Valid file descriptor
     * @return cppcoro::task<int> Task yielding amount of bytes read if > 0 or error code if < 0
     */
    cppcoro::task<int> read_until_full(buffer_t buffer, int fd);

    /**
     * @brief Write to file descriptor
     *
     * @param buffer Non-owning reference to data
     * @param fd Valid file descriptor
     * @return cppcoro::task<int> Task yielding amount of bytes written if > 0 or error code if < 0
     */
    cppcoro::task<int> write(buffer_t buffer, int fd);

    /**
     * @brief Accept connection on socket
     *
     * @param socket Valid file descriptor
     * @return cppcoro::task<int> Task yielding client socket if > 0 or error code if < 0
     */
    cppcoro::task<int> accept(int socket);

    /**
     * @brief Close file descriptor
     *
     * @param fd Valid file descriptor
     * @return cppcoro::task<int> Task yielding 0 on succes and error code < 0 on error
     */
    cppcoro::task<int> close(int fd);

private:
    struct request_t;

    request_t* create_request();
    void delete_request(request_t* request);

    void process();

    void read(buffer_t buffer, int fd, std::coroutine_handle<> coro, int* result);
    void write(buffer_t buffer, int fd, std::coroutine_handle<> coro, int* result);
    void accept(int socket, std::coroutine_handle<> coro, int* result);
    void close(int fd, std::coroutine_handle<> coro, int* result);

    std::unique_ptr<struct io_uring> ring_ { nullptr };
    size_t queue_size_ {};
    size_t queue_min_space_left_ {};
    size_t submissions_ {};
};

}
