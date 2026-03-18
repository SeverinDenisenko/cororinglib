#include "cororing/buffer.hpp"
#include "cororing/ring.hpp"
#include "cororing/sockets.hpp"

#include <iostream>
#include <vector>

#include "cppcoro/async_scope.hpp"
#include "cppcoro/sync_wait.hpp"
#include "cppcoro/task.hpp"
#include "cppcoro/when_all_ready.hpp"

#include "echo_common.hpp"

cppcoro::task<void> handle_connection(cororing::ring_t& ring, int client_socket)
{
    std::vector<std::byte> buffer(100);

    while (true) {
        int len = co_await ring.receive(cororing::buffer_t(buffer), client_socket);

        if (len < 0) {
            std::cout << "Can't read from client." << std::endl;
            co_return;
        }

        if (len == 0) {
            std::cout << "Client disconnected." << std::endl;

            co_await ring.close(client_socket);

            co_return;
        }

        len = co_await ring.send(cororing::const_buffer_t(buffer.data(), len), client_socket);

        if (len <= 0) {
            std::cout << "Can't write to client." << std::endl;
            co_return;
        }
    }
}

cppcoro::task<void> linstner(cororing::ring_t& ring)
{
    cppcoro::async_scope scope;

    int listen_socket = cororing::create_server_socket(
        cororing::socket_protocol_t::tcp, cororing::socket_ip_version_t::ipv4, g_port, "0.0.0.0", 100);

    while (true) {
        int client_socket = co_await ring.accept(listen_socket);

        if (client_socket < 0) {
            std::cout << "Can't accept client." << std::endl;
            continue;
        }

        scope.spawn(handle_connection(ring, client_socket));
    }

    co_await scope.join();
}

cppcoro::task<void> poller(cororing::ring_t& ring)
{
    while (true) {
        ring.poll();
        co_await std::suspend_never {};
    }
}

int main()
{
    cororing::ring_t ring(100);

    cppcoro::sync_wait(cppcoro::when_all_ready(linstner(ring), poller(ring)));

    return 0;
}
