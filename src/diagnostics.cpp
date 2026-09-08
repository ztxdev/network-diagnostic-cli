#include "ztxdiag/diagnostics.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <set>
#include <sstream>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ztxdiag {
namespace {

#if defined(_WIN32)
using SocketHandle = SOCKET;
constexpr SocketHandle invalid_socket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle invalid_socket = -1;
#endif

class SocketRuntime {
public:
    SocketRuntime()
    {
#if defined(_WIN32)
        WSADATA data{};
        ok_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
        ok_ = true;
#endif
    }

    ~SocketRuntime()
    {
#if defined(_WIN32)
        if (ok_) {
            WSACleanup();
        }
#endif
    }

    SocketRuntime(const SocketRuntime&) = delete;
    SocketRuntime& operator=(const SocketRuntime&) = delete;

    [[nodiscard]] bool ok() const noexcept
    {
        return ok_;
    }

private:
    bool ok_{false};
};

class SocketGuard {
public:
    explicit SocketGuard(SocketHandle socket) : socket_(socket) {}

    ~SocketGuard()
    {
        close();
    }

    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;

    [[nodiscard]] SocketHandle get() const noexcept
    {
        return socket_;
    }

    void close() noexcept
    {
        if (socket_ == invalid_socket) {
            return;
        }

#if defined(_WIN32)
        closesocket(socket_);
#else
        ::close(socket_);
#endif
        socket_ = invalid_socket;
    }

private:
    SocketHandle socket_{invalid_socket};
};

std::string socket_error_text()
{
#if defined(_WIN32)
    return "socket error " + std::to_string(WSAGetLastError());
#else
    return std::strerror(errno);
#endif
}

std::string gai_error_text(int code)
{
#if defined(_WIN32)
    return "getaddrinfo error " + std::to_string(code);
#else
    const char* text = gai_strerror(code);
    return text != nullptr ? std::string{text} : "getaddrinfo error " + std::to_string(code);
#endif
}

bool set_nonblocking(SocketHandle socket)
{
#if defined(_WIN32)
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    const int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool is_connect_in_progress()
{
#if defined(_WIN32)
    const int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == WSAEINVAL;
#else
    return errno == EINPROGRESS || errno == EWOULDBLOCK;
#endif
}

std::string sockaddr_to_text(const sockaddr* address)
{
    if (address == nullptr) {
        return {};
    }

    std::array<char, INET6_ADDRSTRLEN> buffer{};

    if (address->sa_family == AF_INET) {
        const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(address);
        if (inet_ntop(AF_INET, &ipv4->sin_addr, buffer.data(),
                      buffer.size()) != nullptr) {
            return buffer.data();
        }
    }

    if (address->sa_family == AF_INET6) {
        const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(address);
        if (inet_ntop(AF_INET6, &ipv6->sin6_addr, buffer.data(),
                      buffer.size()) != nullptr) {
            return buffer.data();
        }
    }

    return {};
}

std::string read_env_value(const char* name)
{
#if defined(_WIN32)
    char* value = nullptr;
    std::size_t length = 0;

    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result{value};
    std::free(value);
    return result;
#else
    if (const char* value = std::getenv(name); value != nullptr) {
        return value;
    }
    return {};
#endif
}

std::string read_env(const char* upper, const char* lower)
{
    if (std::string value = read_env_value(upper); !value.empty()) {
        return value;
    }

    return read_env_value(lower);
}

bool socket_family_available(int family)
{
    const SocketHandle socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (socket == invalid_socket) {
        return false;
    }

#if defined(_WIN32)
    closesocket(socket);
#else
    ::close(socket);
#endif
    return true;
}

std::vector<std::string> resolve_addresses(const std::string& host, std::string& error)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* raw = nullptr;
    const int result = getaddrinfo(host.c_str(), nullptr, &hints, &raw);
    if (result != 0) {
        error = gai_error_text(result);
        return {};
    }

    std::set<std::string> unique;
    for (addrinfo* current = raw; current != nullptr; current = current->ai_next) {
        const std::string address = sockaddr_to_text(current->ai_addr);
        if (!address.empty()) {
            unique.insert(address);
        }
    }

    freeaddrinfo(raw);
    return {unique.begin(), unique.end()};
}

} // namespace

DnsResult resolve_dns(const std::string& host)
{
    DnsResult result;
    result.host = host;

    if (host.empty()) {
        result.error = "host is empty";
        return result;
    }

    SocketRuntime runtime;
    if (!runtime.ok()) {
        result.error = "failed to initialize socket runtime";
        return result;
    }

    result.addresses = resolve_addresses(host, result.error);
    result.success = !result.addresses.empty();

    if (!result.success && result.error.empty()) {
        result.error = "no addresses returned";
    }

    return result;
}

TcpResult check_tcp(
    const std::string& host,
    std::uint16_t port,
    std::chrono::milliseconds timeout)
{
    TcpResult result;
    result.host = host;
    result.port = port;

    if (host.empty()) {
        result.error = "host is empty";
        return result;
    }

    SocketRuntime runtime;
    if (!runtime.ok()) {
        result.error = "failed to initialize socket runtime";
        return result;
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* raw = nullptr;
    const std::string service = std::to_string(port);
    const int resolve_result = getaddrinfo(host.c_str(), service.c_str(), &hints, &raw);

    if (resolve_result != 0) {
        result.error = gai_error_text(resolve_result);
        return result;
    }

    std::string last_error = "connection failed";

    for (addrinfo* current = raw; current != nullptr; current = current->ai_next) {
        const SocketHandle socket =
            ::socket(current->ai_family, current->ai_socktype, current->ai_protocol);

        if (socket == invalid_socket) {
            last_error = socket_error_text();
            continue;
        }

        SocketGuard guard{socket};

        if (!set_nonblocking(socket)) {
            last_error = "failed to enable non-blocking mode: " + socket_error_text();
            continue;
        }

        const auto started = std::chrono::steady_clock::now();
        const int connect_result =
            ::connect(socket, current->ai_addr, static_cast<int>(current->ai_addrlen));

        if (connect_result != 0 && !is_connect_in_progress()) {
            last_error = socket_error_text();
            continue;
        }

        if (connect_result != 0) {
            fd_set write_set;
            FD_ZERO(&write_set);
            FD_SET(socket, &write_set);

            timeval tv{};
            const auto timeout_ms = std::max<std::int64_t>(0, timeout.count());
            tv.tv_sec = static_cast<long>(timeout_ms / 1000);
            tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);

#if defined(_WIN32)
            const int select_result = select(0, nullptr, &write_set, nullptr, &tv);
#else
            const int select_result = select(socket + 1, nullptr, &write_set, nullptr, &tv);
#endif

            if (select_result == 0) {
                last_error = "connection timed out";
                continue;
            }

            if (select_result < 0) {
                last_error = "select failed: " + socket_error_text();
                continue;
            }

            int socket_error = 0;
#if defined(_WIN32)
            int option_length = static_cast<int>(sizeof(socket_error));
#else
            socklen_t option_length = static_cast<socklen_t>(sizeof(socket_error));
#endif

            if (getsockopt(socket, SOL_SOCKET, SO_ERROR,
#if defined(_WIN32)
                           reinterpret_cast<char*>(&socket_error),
#else
                           &socket_error,
#endif
                           &option_length) != 0) {
                last_error = "getsockopt failed: " + socket_error_text();
                continue;
            }

            if (socket_error != 0) {
#if defined(_WIN32)
                last_error = "connect error " + std::to_string(socket_error);
#else
                last_error = std::strerror(socket_error);
#endif
                continue;
            }
        }

        const auto ended = std::chrono::steady_clock::now();
        result.success = true;
        result.connected_address = sockaddr_to_text(current->ai_addr);
        result.latency =
            std::chrono::duration_cast<std::chrono::milliseconds>(ended - started);
        result.error.clear();
        break;
    }

    freeaddrinfo(raw);

    if (!result.success) {
        result.error = last_error;
    }

    return result;
}

EnvironmentResult inspect_environment()
{
    EnvironmentResult result;

    SocketRuntime runtime;
    if (!runtime.ok()) {
        return result;
    }

    std::array<char, 256> host_buffer{};
    if (gethostname(host_buffer.data(), static_cast<int>(host_buffer.size())) == 0) {
        host_buffer.back() = '\0';
        result.hostname = host_buffer.data();

        std::string error;
        result.local_addresses = resolve_addresses(result.hostname, error);
    }

    result.ipv4_stack = socket_family_available(AF_INET);
    result.ipv6_stack = socket_family_available(AF_INET6);

    result.http_proxy = read_env("HTTP_PROXY", "http_proxy");
    result.https_proxy = read_env("HTTPS_PROXY", "https_proxy");
    result.no_proxy = read_env("NO_PROXY", "no_proxy");

    return result;
}

bool parse_port(const std::string& value, std::uint16_t& port)
{
    if (value.empty()) {
        return false;
    }

    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(value, &consumed, 10);

        if (consumed != value.size() || parsed == 0 ||
            parsed > (std::numeric_limits<std::uint16_t>::max)()) {
            return false;
        }

        port = static_cast<std::uint16_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace ztxdiag
