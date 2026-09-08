#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace ztxdiag {

struct HttpTiming {
    long long dns_ms{-1};
    long long tcp_ms{-1};
    long long tls_ms{-1};
    long long ttfb_ms{-1};
    long long total_ms{-1};
};

struct HttpResult {
    bool success{false};
    bool secure{false};
    bool redirected{false};

    std::string url;
    std::string effective_url;
    std::string scheme;
    std::string host;
    std::uint16_t port{0};

    long status_code{0};
    long redirect_count{-1};

    std::string remote_address;
    std::string http_version;
    std::string tls_backend;
    std::string proxy_hint;

    HttpTiming timing;
    std::string error;
};

HttpResult check_http(
    const std::string& url,
    std::chrono::milliseconds timeout = std::chrono::milliseconds{5000});

} // namespace ztxdiag
