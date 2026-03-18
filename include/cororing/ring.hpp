#pragma once

#include "cppcoro/task.hpp"

#include "cororing/buffer.hpp"

#include <cstddef>
#include <memory>
#include <vector>

/** @file */

struct io_uring;
struct io_uring_sqe;

namespace cororing {

/**
 * @brief
 *
 * @return true if io_uring supported and enabled on current platform
 * @return false if error was encountered during looking for io_uring
 */
bool is_io_uring_supported();

/**
 * @brief io_uring coroutine wrapper
 */
class ring_t {
public:
    /**
     * @brief Construct a new ring object
     *
     * @param queue_size Length of submission and completion queues. Must be bigger then 0.
     * @param queue_min_space_left Minimum amount of submission queue free space while processing completion queue.
     * Default is 0 (completion queue will be drained until submission queue will be completely full). Must be smaller
     * then queue_size.
     */
    ring_t(size_t queue_size, size_t queue_min_space_left = 0);

    /**
     * @brief Destroy the ring
     * @warning Do not delete ring object if there are coroutines in suspended state waiting for response!
     */
    ~ring_t();

    ring_t(const ring_t& other)            = delete;
    ring_t& operator=(const ring_t& other) = delete;

    /**
     * @brief Move constructor
     * @warning Do not move ring object if there are coroutines in suspended state waiting for response!
     * @param other
     */
    ring_t(ring_t&& other) = default;

    /**
     * @brief Move assignment operator
     *
     * @param other
     * @return ring_t&
     * @warning Do not move ring object if there are coroutines in suspended state waiting for response!
     */
    ring_t& operator=(ring_t&& other) = default;

    /**
     * @brief Read events from completion queue and resume all coroutines awaiting responses
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
     * @return cppcoro::task<int> Task yielding amount of bytes read if > 0 or error code if < 0
     */
    cppcoro::task<int> read(buffer_t buffer, int fd);

    /**
     * @brief Read exactly buffer.size() bytes from file descriptor
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
    cppcoro::task<int> write(const_buffer_t buffer, int fd);

    /**
     * @brief Write to socket
     *
     * @param buffer Non-owning reference to data
     * @param socket Valid socket
     * @return cppcoro::task<int> Task yielding amount of bytes written if > 0 or error code if < 0
     */
    cppcoro::task<int> send(const_buffer_t buffer, int socket);

    /**
     * @brief Read from socket
     *
     * @param buffer Non-owning reference to data
     * @param socket Valid socket
     * @return cppcoro::task<int>
     */
    cppcoro::task<int> receive(buffer_t buffer, int socket);

    /**
     * @brief Read exactly buffer.size() bytes from socket
     *
     * @param buffer Non-owning reference to data
     * @param fd Valid file descriptor
     * @return cppcoro::task<int> Task yielding amount of bytes read if > 0 or error code if < 0
     */
    cppcoro::task<int> receive_until_full(buffer_t buffer, int fd);

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
    void process();

    template <typename PrepareSqe>
    cppcoro::task<int> submit(PrepareSqe prepare_sqe);

    std::unique_ptr<struct io_uring> ring_ { nullptr };
    size_t queue_size_ { };
    size_t queue_min_space_left_ { };
    size_t submissions_ { };
};

}
