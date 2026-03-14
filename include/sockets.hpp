#pragma once

#include <cstdint>

namespace cororing {

using socket_t = int;

enum class socket_protocol_t {
    tcp,
    udp,
};

enum class socket_ip_version_t {
    ipv4,
    ipv6,
};

socket_t create_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion);

void bind_socket(socket_t socket, socket_ip_version_t ip_vesrion, uint16_t port, const char* address);

void connect_socket(socket_t socket, socket_ip_version_t ip_vesrion, uint16_t port, const char* address);

void listen_socket(socket_t socket, std::size_t backlog);

socket_t
create_server_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion, uint16_t port, const char* address, std::size_t backlog);

socket_t
create_client_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion, uint16_t port, const char* address);

}
