#include "buffer.hpp"
#include "ring.hpp"
#include "sockets.hpp"

#include <iostream>
#include <random>

#include "cppcoro/sync_wait.hpp"
#include "cppcoro/task.hpp"
#include "cppcoro/when_all_ready.hpp"

#include "echo_common.hpp"

static const std::string g_characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
std::random_device g_random_device;
std::mt19937 g_generator(g_random_device());
std::uniform_int_distribution<> g_distribution(0, g_characters.size() - 1);

std::string random_string(std::size_t length)
{
    std::string result;
    for (std::size_t i = 0; i < length; ++i) {
        result += g_characters[g_distribution(g_generator)];
    }
    return result;
}

cppcoro::task<void> spam_strings(cororing::ring_t& ring)
{
    int socket = cororing::create_client_socket(
        cororing::socket_protocol_t::tcp, cororing::socket_ip_version_t::ipv4, g_port, "127.0.0.1");

    size_t requests = 1000;

    for (size_t n = 0; n < requests; ++n) {
        size_t len = 150;

        std::string request = random_string(len);
        std::string response;
        response.resize(len);

        int res = co_await ring.write(cororing::buffer_t(request), socket);

        if (res <= 0) {
            std::cout << "Can't write" << std::endl;
            co_return;
        }

        res = co_await ring.read_until_full(cororing::buffer_t(response), socket);

        if (res <= 0) {
            std::cout << "Can't read" << std::endl;
            co_return;
        }

        response.resize(res);

        if (request != response) {
            std::cout << "Read garbage" << std::endl;
            co_return;
        }
    }

    std::cout << "Finished" << std::endl;
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

    cppcoro::sync_wait(cppcoro::when_all_ready(spam_strings(ring), poller(ring)));

    return 0;
}
