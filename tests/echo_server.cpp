#include "iouring.hpp"

#include <coroutine>

#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <vector>

#include "cppcoro/async_scope.hpp"
#include "cppcoro/sync_wait.hpp"
#include "cppcoro/task.hpp"
#include "cppcoro/when_all_ready.hpp"

int create_listen_socket()
{
    int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
    int enable        = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        return -1;
    }

    int backlog = 10;
    int port    = 1234;

    struct sockaddr_in srv_addr = {};
    srv_addr.sin_family         = AF_INET;
    srv_addr.sin_port           = htons(port);
    srv_addr.sin_addr.s_addr    = htonl(INADDR_ANY);

    if (bind(listen_socket, (const struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        return -1;
    }

    if (listen(listen_socket, backlog) < 0) {
        return -1;
    }

    return listen_socket;
}

cppcoro::task<void> handle_connection(cororing::iouring& ring, int client_socket)
{
    std::vector<std::byte> buffer(100);

    while (true) {
        int len = co_await ring.read(buffer, client_socket);

        if (len < 0) {
            throw std::runtime_error("read");
        }

        if (len == 0) {
            co_return;
        }

        len = co_await ring.write(cororing::buffer_t(buffer.begin(), buffer.begin() + len), client_socket);

        if (len < 0) {
            throw std::runtime_error("write");
        }
    }
}

cppcoro::task<void> linstner(cororing::iouring& ring)
{
    cppcoro::async_scope scope;

    int listen_socket = create_listen_socket();

    if (listen_socket < 0) {
        throw std::runtime_error("create_listen_socket");
    }

    while (true) {
        int client_socket = co_await ring.accept(listen_socket);

        if (client_socket < 0) {
            throw std::runtime_error("client_socket");
        }

        scope.spawn(handle_connection(ring, client_socket));
    }

    co_await scope.join();
}

cppcoro::task<void> poller(cororing::iouring& ring)
{
    while (true) {
        ring.poll();
        co_await std::suspend_never {};
    }
}

int main()
{
    cororing::iouring ring(100);

    cppcoro::sync_wait(cppcoro::when_all_ready(linstner(ring), poller(ring)));

    return 0;
}
