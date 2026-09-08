#include "ztxdiag/format.hpp"

#include <iomanip>
#include <sstream>

namespace ztxdiag {
namespace {

std::string json_bool(bool value)
{
    return value ? "true" : "false";
}

std::string json_string(const std::string& value)
{
    return "\"" + json_escape(value) + "\"";
}

std::string addresses_json(const std::vector<std::string>& addresses)
{
    std::ostringstream out;
    out << '[';

    for (std::size_t i = 0; i < addresses.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << json_string(addresses[i]);
    }

    out << ']';
    return out.str();
}

std::string display_optional(const std::string& value)
{
    return value.empty() ? "(not set)" : value;
}

std::string display_ms(long long value)
{
    if (value < 0) {
        return "n/a";
    }

    return std::to_string(value) + " ms";
}

} // namespace

std::string json_escape(const std::string& value)
{
    std::ostringstream out;

    for (const unsigned char ch : value) {
        switch (ch) {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\b':
            out << "\\b";
            break;
        case '\f':
            out << "\\f";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            if (ch < 0x20U) {
                out << "\\u"
                    << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(ch)
                    << std::dec << std::setfill(' ');
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }

    return out.str();
}

std::string format_dns_text(const DnsResult& result)
{
    std::ostringstream out;
    out << "DNS\n";

    if (!result.success) {
        out << "  FAIL " << result.host << '\n'
            << "  error: " << result.error << '\n';
        return out.str();
    }

    for (const auto& address : result.addresses) {
        out << "  OK   " << address << '\n';
    }

    return out.str();
}

std::string format_dns_json(const DnsResult& result)
{
    std::ostringstream out;
    out << "{"
        << "\"command\":\"dns\","
        << "\"success\":" << json_bool(result.success) << ','
        << "\"host\":" << json_string(result.host) << ','
        << "\"addresses\":" << addresses_json(result.addresses) << ','
        << "\"error\":" << json_string(result.error)
        << "}";
    return out.str();
}

std::string format_tcp_text(const TcpResult& result)
{
    std::ostringstream out;
    out << "TCP\n";

    if (!result.success) {
        out << "  FAIL " << result.host << ':' << result.port << '\n'
            << "  error: " << result.error << '\n';
        return out.str();
    }

    out << "  OK   connected\n"
        << "  address: " << result.connected_address << '\n'
        << "  port: " << result.port << '\n'
        << "  latency: " << result.latency.count() << " ms\n";

    return out.str();
}

std::string format_tcp_json(const TcpResult& result)
{
    std::ostringstream out;
    out << "{"
        << "\"command\":\"tcp\","
        << "\"success\":" << json_bool(result.success) << ','
        << "\"host\":" << json_string(result.host) << ','
        << "\"port\":" << result.port << ','
        << "\"connected_address\":" << json_string(result.connected_address) << ','
        << "\"latency_ms\":" << result.latency.count() << ','
        << "\"error\":" << json_string(result.error)
        << "}";
    return out.str();
}

std::string format_environment_text(const EnvironmentResult& result)
{
    std::ostringstream out;
    out << "ENVIRONMENT\n"
        << "  hostname: " << display_optional(result.hostname) << '\n'
        << "  IPv4 stack: "
        << (result.ipv4_stack ? "available" : "unavailable") << '\n'
        << "  IPv6 stack: "
        << (result.ipv6_stack ? "available" : "unavailable") << '\n'
        << "  HTTP_PROXY: " << display_optional(result.http_proxy) << '\n'
        << "  HTTPS_PROXY: " << display_optional(result.https_proxy) << '\n'
        << "  NO_PROXY: " << display_optional(result.no_proxy) << '\n';

    if (!result.local_addresses.empty()) {
        out << "  local addresses:\n";
        for (const auto& address : result.local_addresses) {
            out << "    - " << address << '\n';
        }
    }

    return out.str();
}

std::string format_environment_json(const EnvironmentResult& result)
{
    std::ostringstream out;
    out << "{"
        << "\"command\":\"env\","
        << "\"hostname\":" << json_string(result.hostname) << ','
        << "\"local_addresses\":" << addresses_json(result.local_addresses) << ','
        << "\"ipv4_stack\":" << json_bool(result.ipv4_stack) << ','
        << "\"ipv6_stack\":" << json_bool(result.ipv6_stack) << ','
        << "\"http_proxy\":" << json_string(result.http_proxy) << ','
        << "\"https_proxy\":" << json_string(result.https_proxy) << ','
        << "\"no_proxy\":" << json_string(result.no_proxy)
        << "}";
    return out.str();
}

std::string format_http_text(const HttpResult& result)
{
    std::ostringstream out;

    out << "HTTP/HTTPS\n"
        << "  URL: " << result.url << '\n';

    if (!result.success) {
        out << "  FAIL\n"
            << "  error: " << result.error << '\n';

        if (!result.host.empty()) {
            out << "  host: " << result.host << '\n'
                << "  port: " << result.port << '\n';
        }

        return out.str();
    }

    out << "  OK   response received\n"
        << "  status: " << result.status_code << '\n'
        << "  protocol: " << display_optional(result.http_version) << '\n'
        << "  effective URL: " << display_optional(result.effective_url) << '\n'
        << "  remote address: " << display_optional(result.remote_address) << '\n'
        << "  redirected: " << (result.redirected ? "yes" : "no") << '\n'
        << "  TLS backend: " << display_optional(result.tls_backend) << '\n'
        << "  proxy hint: " << display_optional(result.proxy_hint) << '\n'
        << "\nTIMING\n"
        << "  DNS: " << display_ms(result.timing.dns_ms) << '\n'
        << "  TCP: " << display_ms(result.timing.tcp_ms) << '\n'
        << "  TLS: " << display_ms(result.timing.tls_ms) << '\n'
        << "  TTFB: " << display_ms(result.timing.ttfb_ms) << '\n'
        << "  total: " << display_ms(result.timing.total_ms) << '\n';

    if (result.secure && result.timing.tls_ms < 0) {
        out << "  note: exact TLS timing is not exposed by the Windows "
               "WinHTTP backend\n";
    }

    return out.str();
}

std::string format_http_json(const HttpResult& result)
{
    std::ostringstream out;

    out << "{"
        << "\"command\":\"http\","
        << "\"success\":" << json_bool(result.success) << ','
        << "\"url\":" << json_string(result.url) << ','
        << "\"effective_url\":" << json_string(result.effective_url) << ','
        << "\"scheme\":" << json_string(result.scheme) << ','
        << "\"host\":" << json_string(result.host) << ','
        << "\"port\":" << result.port << ','
        << "\"secure\":" << json_bool(result.secure) << ','
        << "\"status_code\":" << result.status_code << ','
        << "\"redirected\":" << json_bool(result.redirected) << ','
        << "\"redirect_count\":" << result.redirect_count << ','
        << "\"remote_address\":" << json_string(result.remote_address) << ','
        << "\"http_version\":" << json_string(result.http_version) << ','
        << "\"tls_backend\":" << json_string(result.tls_backend) << ','
        << "\"proxy_hint\":" << json_string(result.proxy_hint) << ','
        << "\"timing\":{"
        << "\"dns_ms\":" << result.timing.dns_ms << ','
        << "\"tcp_ms\":" << result.timing.tcp_ms << ','
        << "\"tls_ms\":" << result.timing.tls_ms << ','
        << "\"ttfb_ms\":" << result.timing.ttfb_ms << ','
        << "\"total_ms\":" << result.timing.total_ms
        << "},"
        << "\"error\":" << json_string(result.error)
        << "}";

    return out.str();
}

std::string format_check_text(
    const DnsResult& dns,
    const TcpResult& tcp,
    const EnvironmentResult& env)
{
    std::ostringstream out;

    out << "ZTXDIAG\n"
        << "=======\n\n"
        << format_dns_text(dns) << '\n'
        << format_tcp_text(tcp) << '\n'
        << format_environment_text(env) << '\n'
        << "RESULT\n"
        << "  " << (dns.success && tcp.success ? "PASS" : "FAIL") << '\n';

    return out.str();
}

std::string format_check_json(
    const DnsResult& dns,
    const TcpResult& tcp,
    const EnvironmentResult& env)
{
    std::ostringstream out;

    out << "{"
        << "\"command\":\"check\","
        << "\"success\":" << json_bool(dns.success && tcp.success) << ','
        << "\"dns\":" << format_dns_json(dns) << ','
        << "\"tcp\":" << format_tcp_json(tcp) << ','
        << "\"environment\":" << format_environment_json(env)
        << "}";

    return out.str();
}

} // namespace ztxdiag
