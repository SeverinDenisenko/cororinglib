#include "cororing/ring.hpp"

#include "cppcoro/single_consumer_event.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <system_error>

#include <liburing.h>
#include <liburing/io_uring.h>
#include <sys/socket.h>

namespace cororing {

namespace {
    struct io_uring_sqe* get_sqe(io_uring* ring)
    {
        struct io_uring_sqe* sqe = io_uring_get_sqe(ring);
        while (!sqe) {
            io_uring_submit(ring);
            sqe = io_uring_get_sqe(ring);
        }
        return sqe;
    }

    struct request_t {
        int refcount { 0 };
        bool cancelled { false };
        cppcoro::single_consumer_event event {};
        int result { -EINVAL };
        std::array<iovec, 1> iov {};
        struct msghdr msg { };

        void acquire()
        {
            refcount++;
        }

        void release()
        {
            refcount--;
            if (refcount == 0) {
                delete this;
            }
        }
    };

    class cancellation_guard {
    public:
        cancellation_guard(request_t* request)
            : request_(request)
        {
        }

        void disarm()
        {
            disarmed_ = true;
        }

        ~cancellation_guard()
        {
            if (!disarmed_) {
                request_->cancelled = true;
                request_->release();
            }
        }

    private:
        request_t* request_ { nullptr };
        bool disarmed_ { false };
    };
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
    int rc = io_uring_queue_init(queue_size_, ring_.get(), 0);
    if (rc < 0) {
        throw std::system_error(-rc, std::system_category(), "io_uring_queue_init");
    }
}

ring_t::~ring_t()
{
    if (ring_) {
        io_uring_queue_exit(ring_.get());
    }
}

void ring_t::process()
{
    if (submissions_ > 0) {
        int rc = io_uring_submit(ring_.get());
        if (rc < 0) {
            throw std::system_error(-rc, std::system_category(), "io_uring_submit");
        }
        submissions_ -= rc;
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

        if (!request->cancelled) {
            request->result = completion_queue->res;
            request->event.set();
        }
        request->release();

        io_uring_cqe_seen(ring_.get(), completion_queue);

        if (io_uring_sq_space_left(ring_.get()) <= queue_min_space_left_) {
            break;
        }

        if (io_uring_peek_cqe(ring_.get(), &completion_queue) == -EAGAIN) {
            break;
        }
    }

    process();

    return true;
}

template <typename PrepareSqe>
cppcoro::task<int> ring_t::submit(PrepareSqe prepare_sqe)
{
    request_t* request = new request_t {};
    request->acquire();

    cancellation_guard guard { request };

    struct io_uring_sqe* sqe = get_sqe(ring_.get());
    prepare_sqe(sqe, request);

    io_uring_sqe_set_data(sqe, request);
    request->acquire();

    submissions_ += 1;

    co_await request->event;

    int result = request->result;

    guard.disarm();
    request->release();

    co_return result;
}

cppcoro::task<int> ring_t::read(buffer_t buffer, int fd)
{
    return submit([&](struct io_uring_sqe* sqe, request_t* request) {
        request->iov[0].iov_base = buffer.data();
        request->iov[0].iov_len  = buffer.size();
        io_uring_prep_readv(sqe, fd, request->iov.data(), request->iov.size(), -1);
    });
}

cppcoro::task<int> ring_t::read_until_full(buffer_t buffer, int fd)
{
    int read = 0;

    while (read < buffer.size()) {
        buffer_t left(buffer.data() + read, buffer.size() - read);
        int res = co_await this->read(left, fd);

        if (res <= 0) {
            if (read == 0) {
                co_return res;
            } else {
                co_return read;
            }
        }

        read += res;
    }

    co_return read;
}

cppcoro::task<int> ring_t::write(const_buffer_t buffer, int fd)
{
    return submit([&](struct io_uring_sqe* sqe, request_t* request) {
        request->iov[0].iov_base = const_cast<std::byte*>(buffer.data());
        request->iov[0].iov_len  = buffer.size();
        io_uring_prep_writev(sqe, fd, request->iov.data(), request->iov.size(), -1);
    });
}

cppcoro::task<int> ring_t::accept(int socket)
{
    return submit([&](struct io_uring_sqe* sqe, request_t*) { io_uring_prep_accept(sqe, socket, nullptr, 0, 0); });
}

cppcoro::task<int> ring_t::send(const_buffer_t buffer, int socket)
{
    return submit([&](struct io_uring_sqe* sqe, request_t* request) {
        request->iov[0].iov_base = const_cast<std::byte*>(buffer.data());
        request->iov[0].iov_len  = buffer.size();
        request->msg.msg_iov     = request->iov.data();
        request->msg.msg_iovlen  = request->iov.size();
        io_uring_prep_sendmsg(sqe, socket, &request->msg, MSG_NOSIGNAL);
    });
}

cppcoro::task<int> ring_t::receive(buffer_t buffer, int socket)
{
    return submit([&](struct io_uring_sqe* sqe, request_t* request) {
        request->iov[0].iov_base = const_cast<std::byte*>(buffer.data());
        request->iov[0].iov_len  = buffer.size();
        request->msg.msg_iov     = request->iov.data();
        request->msg.msg_iovlen  = request->iov.size();
        io_uring_prep_recvmsg(sqe, socket, &request->msg, MSG_NOSIGNAL);
    });
}

cppcoro::task<int> ring_t::receive_until_full(buffer_t buffer, int fd)
{
    int read = 0;

    while (read < buffer.size()) {
        buffer_t left(buffer.data() + read, buffer.size() - read);
        int res = co_await this->receive(left, fd);

        if (res <= 0) {
            if (read == 0) {
                co_return res;
            } else {
                co_return read;
            }
        }

        read += res;
    }

    co_return read;
}

cppcoro::task<int> ring_t::close(int fd)
{
    return submit([&](struct io_uring_sqe* sqe, request_t*) { io_uring_prep_close(sqe, fd); });
}

}
