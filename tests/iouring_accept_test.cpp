#include "future.hpp"
#include "iouring.hpp"
#include "unittest.hpp"

#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

int create_listen_socket()
{
    int listen_socket = socket(PF_INET, SOCK_STREAM, 0);
    int enable        = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        std::abort();
    }

    int backlog = 10;
    int port    = 1234;

    struct sockaddr_in srv_addr = {};
    srv_addr.sin_family         = AF_INET;
    srv_addr.sin_port           = htons(port);
    srv_addr.sin_addr.s_addr    = htonl(INADDR_ANY);

    if (bind(listen_socket, (const struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0) {
        std::abort();
    }

    if (listen(listen_socket, backlog) < 0) {
        std::abort();
    }

    return listen_socket;
}

void poll(cororing::iouring& ring, co::future<int>& future)
{
    while (true) {
        std::cout << "Polling..." << std::endl;

        ring.poll();

        if (future.ready()) {
            break;
        }
    }
}

SIMPLE_TEST(listen_test)
{
    cororing::iouring ring(100);

    int listen_socket = create_listen_socket();

    auto [future, promise] = co::create_future_promise<int>();
    ring.accept(listen_socket, [promise = std::move(promise)](int result) mutable {
        ASSERT_TRUE(result > 0);
        promise.set_value(result);
    });

    poll(ring, future);

    int client_socket = future.get();
    ASSERT_TRUE(client_socket > 0);

    std::string expected = "Hello, io_uring!";
    std::string message;
    message.resize(expected.size());
    cororing::buffer_t buffer = { reinterpret_cast<std::byte*>(message.data()), message.size() };

    auto [future2, promise2] = co::create_future_promise<int>();

    ring.read(buffer, client_socket, [promise2 = std::move(promise2)](int result) mutable {
        promise2.set_value(result); //
    });

    poll(ring, future2);

    ASSERT_TRUE(future2.get() > 0);
    ASSERT_EQ(size_t(future2.get()), expected.size());
    ASSERT_EQ(expected, message);
}

TEST_MAIN()
