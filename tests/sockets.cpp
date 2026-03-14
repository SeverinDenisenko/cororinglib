#include "sockets.hpp"
#include "unittest.hpp"

using namespace cororing;

SIMPLE_TEST(create_sockets)
{
    {
        socket_t server = create_server_socket(socket_protocol_t::tcp, socket_ip_version_t::ipv4, 1234, "0.0.0.0", 100);
        socket_t client = create_client_socket(socket_protocol_t::tcp, socket_ip_version_t::ipv4, 1234, "127.0.0.1");

        close(client);
        close(server);
    }
    {
        socket_t server = create_server_socket(socket_protocol_t::udp, socket_ip_version_t::ipv4, 1234, "0.0.0.0", 0);
        socket_t client = create_client_socket(socket_protocol_t::udp, socket_ip_version_t::ipv4, 1234, "127.0.0.1");

        close(client);
        close(server);
    }
    {
        socket_t server = create_server_socket(socket_protocol_t::tcp, socket_ip_version_t::ipv6, 1234, "::", 100);
        socket_t client = create_client_socket(socket_protocol_t::tcp, socket_ip_version_t::ipv6, 1234, "::1");

        close(client);
        close(server);
    }
    {
        socket_t server = create_server_socket(socket_protocol_t::udp, socket_ip_version_t::ipv6, 1234, "::", 0);
        socket_t client = create_client_socket(socket_protocol_t::udp, socket_ip_version_t::ipv6, 1234, "::1");

        close(client);
        close(server);
    }
}

TEST_MAIN()
