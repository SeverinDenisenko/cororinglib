#include "iouring.hpp"
#include "unittest.hpp"

#include "cppcoro/sync_wait.hpp"
#include "cppcoro/when_all_ready.hpp"
#include "cppcoro/task.hpp"

#include <coroutine>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

int create_listen_socket()
{
    int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
    int enable        = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        ASSERT_TRUE(false);
    }

    int backlog = 10;
    int port    = 1234;

    struct sockaddr_in srv_addr = {};
    srv_addr.sin_family         = AF_INET;
    srv_addr.sin_port           = htons(port);
    srv_addr.sin_addr.s_addr    = htonl(INADDR_ANY);

    if (bind(listen_socket, (const struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        ASSERT_TRUE(false);
    }

    if (listen(listen_socket, backlog) < 0) {
        ASSERT_TRUE(false);
    }

    return listen_socket;
}

cppcoro::task<int> accept_connection(cororing::iouring& ring, int listen_socket)
{
    int client_socket = co_await ring.accept(listen_socket);

    ASSERT_TRUE(client_socket > 0);

    co_return client_socket;
}

cppcoro::task<std::string> read_from_socket(cororing::iouring& ring, int client_socket, int max_size)
{
    std::vector<char> buffer(max_size);

    int size = co_await ring.read(
        cororing::buffer_t { reinterpret_cast<std::byte*>(buffer.data()), buffer.size() }, client_socket);

    ASSERT_TRUE(size > 0);

    co_return std::string(buffer.begin(), buffer.begin() + size);
}

cppcoro::task<void> recive_message(cororing::iouring& ring)
{
    int listen_socket = create_listen_socket();

    int client_socket = co_await accept_connection(ring, listen_socket);

    std::string expected = "Hello, io_uring!";
    std::string message  = co_await read_from_socket(ring, client_socket, expected.size());

    ASSERT_EQ(message, "Hello, io_uring!");
}

SIMPLE_TEST(listen_test)
{
    cororing::iouring ring(100);

    cppcoro::sync_wait(cppcoro::when_all_ready(recive_message(ring), [&]() -> cppcoro::task<void> {
        int acc = 0;
        while (acc < 2) {
            if (ring.poll()) {
                acc++;
            }
            co_await std::suspend_never {};
        }
    }()));
}

TEST_MAIN()
