#include "ztxdiag/diagnostics.hpp"
#include "ztxdiag/format.hpp"
#include "ztxdiag/http.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main()
{
    bool ok = true;

    std::uint16_t port = 0;

    ok &= expect(ztxdiag::parse_port("443", port),
                 "443 must be a valid port");
    ok &= expect(port == 443, "443 must parse to 443");
    ok &= expect(!ztxdiag::parse_port("0", port),
                 "port 0 must be rejected");
    ok &= expect(!ztxdiag::parse_port("65536", port),
                 "port 65536 must be rejected");
    ok &= expect(!ztxdiag::parse_port("abc", port),
                 "non-numeric port must be rejected");

    ok &= expect(
        ztxdiag::json_escape("a\"b\\c\n") == "a\\\"b\\\\c\\n",
        "JSON escaping must handle quotes, backslashes, and newlines");

    const auto localhost = ztxdiag::resolve_dns("localhost");
    ok &= expect(localhost.success, "localhost should resolve");
    ok &= expect(!localhost.addresses.empty(),
                 "localhost must have at least one address");

    const auto env = ztxdiag::inspect_environment();
    ok &= expect(env.ipv4_stack,
                 "IPv4 stack should be available on the runtime host");

    ztxdiag::HttpResult sample;
    sample.success = true;
    sample.url = "https://example.com";
    sample.effective_url = sample.url;
    sample.scheme = "https";
    sample.host = "example.com";
    sample.port = 443;
    sample.secure = true;
    sample.status_code = 200;
    sample.http_version = "HTTP/2";
    sample.tls_backend = "test";
    sample.timing.dns_ms = 1;
    sample.timing.tcp_ms = 2;
    sample.timing.tls_ms = 3;
    sample.timing.ttfb_ms = 4;
    sample.timing.total_ms = 5;

    const auto json = ztxdiag::format_http_json(sample);
    ok &= expect(
        json.find("\"status_code\":200") != std::string::npos,
        "HTTP JSON must include status code");
    ok &= expect(
        json.find("\"tls_ms\":3") != std::string::npos,
        "HTTP JSON must include TLS timing");

    if (!ok) {
        return EXIT_FAILURE;
    }

    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
}
