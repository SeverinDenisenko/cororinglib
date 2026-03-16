#include "cororing/buffer.hpp"
#include "cororing/perftools.hpp"
#include "cororing/ring.hpp"
#include "cororing/sockets.hpp"

#include <iostream>
#include <ostream>
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

cppcoro::task<cororing::perf_sampler> spam_strings(cororing::ring_t& ring, size_t requests, size_t request_size)
{
    int socket = cororing::create_client_socket(
        cororing::socket_protocol_t::tcp, cororing::socket_ip_version_t::ipv4, g_port, "127.0.0.1");

    cororing::perf_sampler sampler {};

    for (size_t n = 0; n < requests; ++n) {
        std::string request = random_string(request_size);

        std::string response;
        response.resize(request_size);

        sampler.begin_sample();

        int res = co_await ring.write(cororing::const_buffer_t(request), socket);

        if (res <= 0) {
            std::cout << "Can't write" << std::endl;
            break;
        }

        res = co_await ring.read_until_full(cororing::buffer_t(response), socket);

        if (res <= 0) {
            std::cout << "Can't read" << std::endl;
            break;
        }

        sampler.end_sample();

        response.resize(res);

        if (request != response) {
            std::cout << "Read garbage" << std::endl;
            break;
        }
    }

    co_await ring.close(socket);

    co_return sampler;
}

cppcoro::task<void> client(cororing::ring_t& ring)
{
    size_t request_size = 1024 * 1024;
    size_t requests     = 10;

    while (true) {
        cororing::perf_sampler sampler = co_await spam_strings(ring, requests, request_size);

        std::cout << "Finished " << requests << " requests of size " << request_size << ": " << std::endl;
        std::cout << "Min: " << sampler.get_min() << std::endl;
        std::cout << "Max: " << sampler.get_max() << std::endl;
        std::cout << "Avg: " << sampler.get_mean() << std::endl;
        std::cout << "Std: " << sampler.get_std() << std::endl;
    }
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

    cppcoro::sync_wait(cppcoro::when_all_ready(client(ring), poller(ring)));

    return 0;
}
