#include "cororing/ring.hpp"
#include "cororing/sockets.hpp"
#include "unittest.hpp"

#include "cppcoro/sync_wait.hpp"
#include "cppcoro/task.hpp"
#include "cppcoro/when_all_ready.hpp"

#include <cstdlib>
#include <cstring>

cppcoro::task<int> accept_connection(cororing::ring_t& ring, int listen_socket)
{
    auto [client_socket, address] = co_await ring.accept(listen_socket);

    ASSERT_EQ(cororing::ipv4_address::to_string(std::get<cororing::ipv4_address>(address)), "127.0.0.1");
    ASSERT_TRUE(client_socket > 0);

    co_return client_socket;
}

cppcoro::task<std::string> read_from_socket(cororing::ring_t& ring, int client_socket, size_t max_size)
{
    std::vector<char> buffer(max_size);

    int size = co_await ring.read(cororing::buffer_t(buffer), client_socket);

    ASSERT_TRUE(size > 0);

    co_return std::string(buffer.begin(), buffer.begin() + size);
}

cppcoro::task<void> recive_message(cororing::ring_t& ring)
{
    int listen_socket = cororing::create_server_socket(
        cororing::socket_protocol_t::tcp, cororing::socket_ip_version_t::ipv4, 1234, "0.0.0.0", 100);

    int client_socket = co_await accept_connection(ring, listen_socket);

    std::string expected = "Hello, io_uring!";
    std::string message  = co_await read_from_socket(ring, client_socket, expected.size());

    ASSERT_EQ(message, "Hello, io_uring!");
}

SIMPLE_TEST(listen_test)
{
    cororing::ring_t ring(100);

    cppcoro::sync_wait([&]() -> cppcoro::task<void> {
        auto [task1, task2] = co_await cppcoro::when_all_ready(recive_message(ring), [&]() -> cppcoro::task<void> {
            int acc = 0;
            while (acc < 2) {
                if (ring.poll()) {
                    acc++;
                }
                co_await std::suspend_never {};
            }
        }());

        task1.result();
        task2.result();
    }());
}

TEST_MAIN()
