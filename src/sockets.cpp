#include "sockets.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <system_error>

namespace cororing {

namespace {
    void throw_error_code(int code)
    {
        throw std::system_error(std::error_code(code, std::generic_category()));
    }

    socket_t create_socket(int domain, int type)
    {
        socket_t sock = socket(domain, type, 0);

        if (sock < 0) {
            throw_error_code(sock);
        }

        int enable = 1;
        int rc     = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
        if (rc < 0) {
            close(sock);
            throw_error_code(rc);
        }

        if (domain == AF_INET6) {
            int ipv6_only = 1;
            int rc        = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only));
            if (rc < 0) {
                close(sock);
                throw_error_code(rc);
            }
        }

        if (type == SOCK_STREAM) {
            int flag = 1;
            int rc   = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
            if (rc < 0) {
                close(sock);
                throw_error_code(rc);
            }
        }

        return sock;
    }

    struct sockaddr_in resolve_name_ipv4(uint16_t port, const char* address)
    {
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);

        int ret = inet_pton(AF_INET, address, &addr.sin_addr);
        if (ret < 0) {
            throw_error_code(ret);
        }

        return addr;
    }

    struct sockaddr_in6 resolve_name_ipv6(uint16_t port, const char* address)
    {
        struct sockaddr_in6 addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port   = htons(port);

        int ret = inet_pton(AF_INET6, address, &addr.sin6_addr);
        if (ret < 0) {
            throw_error_code(ret);
        }

        return addr;
    }

    void connect_socket_ipv4(socket_t socket, uint16_t port, const char* address)
    {
        struct sockaddr_in addr4 = resolve_name_ipv4(port, address);

        int ret = connect(socket, (struct sockaddr*)&addr4, sizeof(addr4));
        if (ret < 0) {
            throw_error_code(errno);
        }
    }

    void connect_socket_ipv6(socket_t socket, uint16_t port, const char* address)
    {
        struct sockaddr_in6 addr = resolve_name_ipv6(port, address);

        int ret = connect(socket, (struct sockaddr*)&addr, sizeof(addr));
        if (ret < 0) {
            throw_error_code(errno);
        }
    }

    void bind_socket_ipv4(socket_t socket, uint16_t port, const char* address)
    {
        struct sockaddr_in addr4 = resolve_name_ipv4(port, address);

        int ret = bind(socket, (struct sockaddr*)&addr4, sizeof(addr4));
        if (ret < 0) {
            throw_error_code(errno);
        }
    }

    void bind_socket_ipv6(socket_t socket, uint16_t port, const char* address)
    {
        struct sockaddr_in6 addr = resolve_name_ipv6(port, address);

        int ret = bind(socket, (struct sockaddr*)&addr, sizeof(addr));
        if (ret < 0) {
            throw_error_code(errno);
        }
    }
}

socket_t create_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion)
{
    int domain;
    int type;

    if (ip_vesrion == socket_ip_version_t::ipv4) {
        domain = AF_INET;
    } else if (ip_vesrion == socket_ip_version_t::ipv6) {
        domain = AF_INET6;
    } else {
        throw_error_code(EINVAL);
    }

    if (protocol == socket_protocol_t::tcp) {
        type = SOCK_STREAM;
    } else if (protocol == socket_protocol_t::udp) {
        type = SOCK_DGRAM;
    } else {
        throw_error_code(EINVAL);
    }

    socket_t result = create_socket(domain, type);

    return result;
}

void bind_socket(socket_t socket, socket_ip_version_t ip_vesrion, uint16_t port, const char* address)
{
    if (ip_vesrion == socket_ip_version_t::ipv4) {
        bind_socket_ipv4(socket, port, address);
    } else if (ip_vesrion == socket_ip_version_t::ipv6) {
        bind_socket_ipv6(socket, port, address);
    } else {
        throw_error_code(EINVAL);
    }
}

void listen_socket(socket_t socket, std::size_t backlog)
{
    int ret = listen(socket, backlog);
    if (ret < 0) {
        throw_error_code(ret);
    }
}

socket_t create_server_socket(
    socket_protocol_t protocol, socket_ip_version_t ip_vesrion, uint16_t port, const char* address, std::size_t backlog)
{
    socket_t socket = create_socket(protocol, ip_vesrion);

    try {
        bind_socket(socket, ip_vesrion, port, address);
    } catch (...) {
        close(socket);
        throw;
    }

    if (protocol == socket_protocol_t::tcp) {
        try {
            listen_socket(socket, backlog);
        } catch (...) {
            close(socket);
            throw;
        }
    } else if (protocol == socket_protocol_t::udp) {
        if (backlog != 0) {
            throw_error_code(EINVAL);
        }
    }

    return socket;
}

void connect_socket(socket_t socket, socket_ip_version_t ip_vesrion, uint16_t port, const char* address)
{
    if (ip_vesrion == socket_ip_version_t::ipv4) {
        connect_socket_ipv4(socket, port, address);
    } else if (ip_vesrion == socket_ip_version_t::ipv6) {
        connect_socket_ipv6(socket, port, address);
    } else {
        throw_error_code(EINVAL);
    }
}

socket_t
create_client_socket(socket_protocol_t protocol, socket_ip_version_t ip_vesrion, uint16_t port, const char* address)
{
    socket_t socket = create_socket(protocol, ip_vesrion);

    try {
        connect_socket(socket, ip_vesrion, port, address);
    } catch (...) {
        close(socket);
        throw;
    }

    return socket;
}

}
