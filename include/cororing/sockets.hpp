#pragma once

#include <cstdint>

/** @file */

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

/**
 * @brief Create a socket
 * 
 * @param protocol socket_protocol_t::tcp or socket_protocol_t::udp
 * @param ip_vesrion socket_ip_version_t::ipv4 or socket_ip_version_t::ipv6
 * @return socket_t valid socket
 * @throws std::system_error if error occured in any part of creating or setuping the socket
 */
socket_t create_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion);

/**
 * @brief bind socket to address
 * 
 * @param socket valid socket
 * @param ip_vesrion socket_ip_version_t::ipv4 or socket_ip_version_t::ipv6
 * @param port 16 bit unsigned int representing local port to bind to
 * @param address string cotaining address like "0.0.0.0" and "127.0.0.1" for socket_ip_version_t::ipv4 or "::" and
 * "::1" for socket_ip_version_t::ipv6
 * @throws std::system_error if error occured in any part of setuping the socket
 */
void bind_socket(socket_t socket, socket_ip_version_t ip_vesrion, uint16_t port, const char* address);

/**
 * @brief Connect to remote
 * 
 * @param socket valid socket
 * @param ip_vesrion socket_ip_version_t::ipv4 or socket_ip_version_t::ipv6
 * @param port 16 bit unsigned int representing port on remote to connect to
 * @param address string cotaining address like "127.0.0.1" for socket_ip_version_t::ipv4 or "::1" for
 * socket_ip_version_t::ipv6
 * @throws std::system_error if error occured in any part of setuping the socket
 */
void connect_socket(socket_t socket, socket_ip_version_t ip_vesrion, uint16_t port, const char* address);

/**
 * @brief Listen tcp socket
 * 
 * @param socket valid tcp socket
 * @param backlog backlog
 * @throws std::system_error if error occured in any part of creating and setuping the socket
 */
void listen_socket(socket_t socket, std::size_t backlog);

/**
 * @brief Create a server socket object
 *
 * @param protocol socket_protocol_t::tcp or socket_protocol_t::udp
 * @param ip_vesrion socket_ip_version_t::ipv4 or socket_ip_version_t::ipv6
 * @param port 16 bit unsigned int representing local port to bind to
 * @param address string cotaining address like "0.0.0.0" and "127.0.0.1" for socket_ip_version_t::ipv4 or "::" and
 * "::1" for socket_ip_version_t::ipv6
 * @param backlog socket backlog for socket_protocol_t::tcp and 0 for socket_protocol_t::udp
 * @return socket_t valid socket
 * @throws std::system_error if error occured in any part of creating and setuping the socket
 * @throws std::system_error if non-zero backlog value was supplied for socket_protocol_t::udp
 */
socket_t create_server_socket(
    socket_protocol_t protocol,
    socket_ip_version_t ip_vesrion,
    uint16_t port,
    const char* address,
    std::size_t backlog);

/**
 * @brief Create a client socket object
 *
 * @param protocol socket_protocol_t::tcp or socket_protocol_t::udp
 * @param ip_vesrion socket_ip_version_t::ipv4 or socket_ip_version_t::ipv6
 * @param port 16 bit unsigned int representing port on remote to connect to
 * @param address string cotaining address like "127.0.0.1" for socket_ip_version_t::ipv4 or "::1" for
 * socket_ip_version_t::ipv6
 * @return socket_t valid socket
 * @throws std::system_error if error occured in any part of creating and setuping the socket
 */
socket_t
create_client_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion, uint16_t port, const char* address);

}
