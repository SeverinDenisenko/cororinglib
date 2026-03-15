#include "cororing/ring.hpp"

#include <array>
#include <coroutine>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>

#include <liburing.h>
#include <liburing/io_uring.h>

namespace cororing {

static const char* g_io_uring_op[] = {
    "IORING_OP_NOP",
    "IORING_OP_READV",
    "IORING_OP_WRITEV",
    "IORING_OP_FSYNC",
    "IORING_OP_READ_FIXED",
    "IORING_OP_WRITE_FIXED",
    "IORING_OP_POLL_ADD",
    "IORING_OP_POLL_REMOVE",
    "IORING_OP_SYNC_FILE_RANGE",
    "IORING_OP_SENDMSG",
    "IORING_OP_RECVMSG",
    "IORING_OP_TIMEOUT",
    "IORING_OP_TIMEOUT_REMOVE",
    "IORING_OP_ACCEPT",
    "IORING_OP_ASYNC_CANCEL",
    "IORING_OP_LINK_TIMEOUT",
    "IORING_OP_CONNECT",
    "IORING_OP_FALLOCATE",
    "IORING_OP_OPENAT",
    "IORING_OP_CLOSE",
    "IORING_OP_FILES_UPDATE",
    "IORING_OP_STATX",
    "IORING_OP_READ",
    "IORING_OP_WRITE",
    "IORING_OP_FADVISE",
    "IORING_OP_MADVISE",
    "IORING_OP_SEND",
    "IORING_OP_RECV",
    "IORING_OP_OPENAT2",
    "IORING_OP_EPOLL_CTL",
    "IORING_OP_SPLICE",
    "IORING_OP_PROVIDE_BUFFERS",
    "IORING_OP_REMOVE_BUFFERS",
    "IORING_OP_TEE",
    "IORING_OP_SHUTDOWN",
    "IORING_OP_RENAMEAT",
    "IORING_OP_UNLINKAT",
    "IORING_OP_MKDIRAT",
    "IORING_OP_SYMLINKAT",
    "IORING_OP_LINKAT",
    "IORING_OP_MSG_RING",
    "IORING_OP_FSETXATTR",
    "IORING_OP_SETXATTR",
    "IORING_OP_FGETXATTR",
    "IORING_OP_GETXATTR",
    "IORING_OP_SOCKET",
    "IORING_OP_URING_CMD",
    "IORING_OP_SEND_ZC",
    "IORING_OP_SENDMSG_ZC",
};

static constexpr size_t g_io_uring_ops = 49;

std::vector<const char*> get_capabilities()
{
    std::vector<const char*> result {};

    struct io_uring_probe* probe = io_uring_get_probe();

    if (!probe) {
        return result;
    }

    size_t ops = std::min(size_t(IORING_OP_LAST), g_io_uring_ops);

    for (char i = 0; i < ops; i++) {
        if (io_uring_opcode_supported(probe, i)) {
            result.push_back(g_io_uring_op[i]);
        }
    }

    io_uring_free_probe(probe);

    return result;
}

bool is_io_uring_supported()
{
    struct io_uring_probe* probe = io_uring_get_probe();

    if (!probe) {
        return false;
    } else {
        io_uring_free_probe(probe);

        return true;
    }
}

ring_t::ring_t(size_t queue_size, size_t queue_min_space_left)
    : ring_(std::make_unique<struct io_uring>())
    , queue_size_(queue_size)
    , queue_min_space_left_(queue_min_space_left)
    , submissions_(0)
{
    if (io_uring_queue_init(queue_size_, ring_.get(), 0) < 0) {
        std::abort();
    }
}

ring_t::~ring_t()
{
    io_uring_queue_exit(ring_.get());
}

enum class request_type_t : size_t {
    READ,
    WRITE,
    ACCEPT,
    CLOSE,
};

struct ring_t::request_t {
    request_type_t type {};
    int* result {};
    std::coroutine_handle<> coro {};
    std::array<iovec, 1> iov {};
};

ring_t::request_t* ring_t::create_request()
{
    return new request_t {};
}

void ring_t::delete_request(ring_t::request_t* request)
{
    delete request;
}

void ring_t::process()
{
    if (submissions_ > 0) {
        io_uring_submit(ring_.get());
        submissions_ = 0;
    }
}

bool ring_t::poll()
{
    process();

    struct __kernel_timespec timeout;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = 1;

    struct io_uring_cqe* completion_queue {};
    int res = io_uring_wait_cqe_timeout(ring_.get(), &completion_queue, &timeout);
    if (res == -ETIME) {
        return false;
    } else if (res < 0) {
        throw std::runtime_error("io_uring_wait_cqe_timeout");
    }

    while (true) {
        request_t* request = (struct request_t*)completion_queue->user_data;
        if (!request) {
            throw std::runtime_error("completion_queue->user_data");
        }

        switch (request->type) {
        case request_type_t::READ:
        case request_type_t::WRITE:
        case request_type_t::ACCEPT:
        case request_type_t::CLOSE:
            *request->result = completion_queue->res;
            request->coro.resume();
            delete_request(request);
            break;
        default:
            throw std::runtime_error("request->type");
            break;
        }

        io_uring_cqe_seen(ring_.get(), completion_queue);

        if (io_uring_sq_space_left(ring_.get()) <= queue_min_space_left_) {
            break; // the submission queue is full
        }

        if (io_uring_peek_cqe(ring_.get(), &completion_queue) == -EAGAIN) {
            break; // no remaining work in completion queue
        }
    }

    process();

    return true;
}

void ring_t::read(buffer_t buffer, int fd, std::coroutine_handle<> coro, int* result)
{
    request_t* request = create_request();

    request->iov[0].iov_base = buffer.data();
    request->iov[0].iov_len  = buffer.size();
    request->type            = request_type_t::READ;
    request->coro            = coro;
    request->result          = result;

    std::memset(request->iov[0].iov_base, 0, request->iov[0].iov_len);

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());
    io_uring_prep_readv(sqe, fd, request->iov.data(), request->iov.size(), 0);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

void ring_t::write(buffer_t buffer, int fd, std::coroutine_handle<> coro, int* result)
{
    request_t* request = create_request();

    request->iov[0].iov_base = buffer.data();
    request->iov[0].iov_len  = buffer.size();
    request->type            = request_type_t::WRITE;
    request->coro            = coro;
    request->result          = result;

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());
    io_uring_prep_writev(sqe, fd, request->iov.data(), request->iov.size(), 0);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

void ring_t::accept(int socket, std::coroutine_handle<> coro, int* result)
{
    request_t* request = create_request();

    request->iov[0].iov_base = 0;
    request->iov[0].iov_len  = 0;
    request->type            = request_type_t::ACCEPT;
    request->coro            = coro;
    request->result          = result;

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());
    io_uring_prep_accept(sqe, socket, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

void ring_t::close(int fd, std::coroutine_handle<> coro, int* result)
{
    request_t* request = create_request();

    request->iov[0].iov_base = 0;
    request->iov[0].iov_len  = 0;
    request->type            = request_type_t::CLOSE;
    request->coro            = coro;
    request->result          = result;

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());
    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

cppcoro::task<int> ring_t::read(buffer_t buffer, int fd)
{
    class awaiter {
    public:
        awaiter(ring_t& ring, buffer_t buffer, int fd)
            : ring_(ring)
            , buffer_(std::move(buffer))
            , client_socket_(fd)
        {
        }

        bool await_ready()
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            ring_.read(buffer_, client_socket_, handle, &value_);
        }

        int await_resume()
        {
            return value_;
        }

    private:
        ring_t& ring_;
        buffer_t buffer_;
        int client_socket_;
        int value_ {};
    };

    co_return co_await awaiter { *this, buffer, fd };
}

cppcoro::task<int> ring_t::read_until_full(buffer_t buffer, int fd)
{
    size_t read = 0;

    while (read < buffer.size()) {
        buffer_t left(buffer.data() + read, buffer.size() - read);
        int res = co_await this->read(left, fd);

        if (res <= 0) {
            co_return res;
        }

        read += res;
    }

    co_return read;
}

cppcoro::task<int> ring_t::write(buffer_t buffer, int fd)
{
    class awaiter {
    public:
        awaiter(ring_t& ring, buffer_t buffer, int fd)
            : ring_(ring)
            , buffer_(std::move(buffer))
            , client_socket_(fd)
        {
        }

        bool await_ready()
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            ring_.write(buffer_, client_socket_, handle, &value_);
        }

        int await_resume()
        {
            return value_;
        }

    private:
        ring_t& ring_;
        buffer_t buffer_;
        int client_socket_;
        int value_ {};
    };

    co_return co_await awaiter { *this, buffer, fd };
}

cppcoro::task<int> ring_t::accept(int socket)
{
    class awaiter {
    public:
        awaiter(ring_t& ring, int socket)
            : ring_(ring)
            , listen_socket_(socket)
        {
        }

        bool await_ready()
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            ring_.accept(listen_socket_, handle, &value_);
        }

        int await_resume()
        {
            return value_;
        }

    private:
        ring_t& ring_;
        int listen_socket_;
        int value_ {};
    };

    co_return co_await awaiter { *this, socket };
}

cppcoro::task<int> ring_t::close(int fd)
{
    class awaiter {
    public:
        awaiter(ring_t& ring, int fd)
            : ring_(ring)
            , client_socket_(fd)
        {
        }

        bool await_ready()
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) noexcept
        {
            ring_.close(client_socket_, handle, &value_);
        }

        int await_resume()
        {
            return value_;
        }

    private:
        ring_t& ring_;
        int client_socket_;
        int value_ {};
    };

    co_return co_await awaiter { *this, fd };
}

}
