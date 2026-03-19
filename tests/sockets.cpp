#include "cororing/sockets.hpp"
#include "unittest.hpp"

using namespace cororing;

SIMPLE_TEST(ip_address)
{
    {
        ipv4_address address = ipv4_address::from_string("127.0.0.1");
        ASSERT_EQ(ipv4_address::to_string(address), "127.0.0.1");
    }
    {
        ipv6_address address = ipv6_address::from_string("::1");
        ASSERT_EQ(ipv6_address::to_string(address), "::1");
    }
}

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
