#include "iouring.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>

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

std::vector<std::string> get_capabilities()
{
    std::vector<std::string> result {};

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

iouring::iouring(size_t queue_size)
    : ring_(std::make_unique<struct io_uring>())
    , queue_size_(queue_size)
    , submissions_(0)
{
    if (io_uring_queue_init(queue_size_, ring_.get(), 0) < 0) {
        std::abort();
    }
}

iouring::~iouring()
{
    io_uring_queue_exit(ring_.get());
}

enum class request_type_t : size_t {
    READ,
    WRITE,
    ACCEPT,
};

struct iouring::request_t {
    request_type_t type {};
    std::array<iovec, 1> iov {};
    co::move_only_function<void, int> callback {};
};

iouring::request_t* iouring::create_request()
{
    return new request_t {};
}

void iouring::delete_request(iouring::request_t* request)
{
    delete request;
}

void iouring::process()
{
    if (submissions_ > 0) {
        io_uring_submit(ring_.get());
        submissions_ = 0;
    }
}

void iouring::poll()
{
    process();

    struct io_uring_cqe* completion_queue {};
    if (io_uring_wait_cqe(ring_.get(), &completion_queue) < 0) {
        std::abort();
    }

    while (true) {
        request_t* request = (struct request_t*)completion_queue->user_data;
        if (!request) {
            std::abort();
        }

        switch (request->type) {
        case request_type_t::READ:
        case request_type_t::WRITE:
        case request_type_t::ACCEPT:
            if (request->callback) {
                request->callback(completion_queue->res);
            }
            delete_request(request);
            break;
        default:
            std::abort();
            break;
        }

        io_uring_cqe_seen(ring_.get(), completion_queue);

        if (io_uring_sq_space_left(ring_.get()) < queue_size_ / 2) {
            break; // the submission queue is full
        }

        if (io_uring_peek_cqe(ring_.get(), &completion_queue) == -EAGAIN) {
            break; // no remaining work in completion queue
        }
    }

    process();
}

void iouring::read(buffer_t buffer, int client_socket, co::move_only_function<void, int> callback)
{
    request_t* request = create_request();

    request->iov[0].iov_base = buffer.data();
    request->iov[0].iov_len  = buffer.size();
    request->type            = request_type_t::READ;
    request->callback        = std::move(callback);

    std::memset(request->iov[0].iov_base, 0, request->iov[0].iov_len);

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());
    io_uring_prep_readv(sqe, client_socket, request->iov.data(), request->iov.size(), 0);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

void iouring::write(buffer_t buffer, int client_socket, co::move_only_function<void, int> callback)
{
    request_t* request = create_request();

    request->iov[0].iov_base = buffer.data();
    request->iov[0].iov_len  = buffer.size();
    request->type            = request_type_t::WRITE;
    request->callback        = std::move(callback);

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());
    io_uring_prep_writev(sqe, client_socket, request->iov.data(), request->iov.size(), 0);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

void iouring::accept(int listen_socket, co::move_only_function<void, int> callback)
{
    request_t* request = create_request();
    request->type      = request_type_t::ACCEPT;
    request->callback  = std::move(callback);

    struct io_uring_sqe* sqe = io_uring_get_sqe(ring_.get());

    io_uring_prep_accept(sqe, listen_socket, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, request);

    submissions_ += 1;
}

}
