#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace ztxdiag {

struct DnsResult {
    bool success{false};
    std::string host;
    std::vector<std::string> addresses;
    std::string error;
};

struct TcpResult {
    bool success{false};
    std::string host;
    std::uint16_t port{0};
    std::string connected_address;
    std::chrono::milliseconds latency{0};
    std::string error;
};

struct EnvironmentResult {
    std::string hostname;
    std::vector<std::string> local_addresses;
    bool ipv4_stack{false};
    bool ipv6_stack{false};
    std::string http_proxy;
    std::string https_proxy;
    std::string no_proxy;
};

DnsResult resolve_dns(const std::string& host);

TcpResult check_tcp(
    const std::string& host,
    std::uint16_t port,
    std::chrono::milliseconds timeout = std::chrono::milliseconds{3000});

EnvironmentResult inspect_environment();

bool parse_port(const std::string& value, std::uint16_t& port);

} // namespace ztxdiag
