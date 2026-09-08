#include "http_internal.hpp"

#include "ztxdiag/diagnostics.hpp"

#include <limits>
#include <string>

namespace ztxdiag::detail {
namespace {

bool parse_port_text(const std::string& value, std::uint16_t& port)
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

} // namespace

ParsedHttpUrl parse_http_url(const std::string& url)
{
    ParsedHttpUrl result;

    const std::size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        result.error = "URL must include http:// or https://";
        return result;
    }

    result.scheme = url.substr(0, scheme_end);
    if (result.scheme == "http") {
        result.secure = false;
        result.port = 80;
    } else if (result.scheme == "https") {
        result.secure = true;
        result.port = 443;
    } else {
        result.error = "unsupported URL scheme: " + result.scheme;
        return result;
    }

    const std::size_t authority_start = scheme_end + 3;
    const std::size_t path_start = url.find_first_of("/?#", authority_start);

    std::string authority =
        path_start == std::string::npos
            ? url.substr(authority_start)
            : url.substr(authority_start, path_start - authority_start);

    if (authority.empty()) {
        result.error = "URL host is empty";
        return result;
    }

    if (authority.find('@') != std::string::npos) {
        result.error = "credentials in URLs are not supported";
        return result;
    }

    if (authority.front() == '[') {
        const std::size_t close = authority.find(']');
        if (close == std::string::npos) {
            result.error = "invalid bracketed IPv6 host";
            return result;
        }

        result.host = authority.substr(1, close - 1);

        if (close + 1 < authority.size()) {
            if (authority[close + 1] != ':') {
                result.error = "invalid IPv6 authority";
                return result;
            }

            if (!parse_port_text(authority.substr(close + 2), result.port)) {
                result.error = "invalid URL port";
                return result;
            }
        }
    } else {
        const std::size_t colon = authority.rfind(':');

        if (colon != std::string::npos) {
            if (authority.find(':') != colon) {
                result.error = "IPv6 literals must use [address] syntax";
                return result;
            }

            result.host = authority.substr(0, colon);

            if (!parse_port_text(authority.substr(colon + 1), result.port)) {
                result.error = "invalid URL port";
                return result;
            }
        } else {
            result.host = authority;
        }
    }

    if (result.host.empty()) {
        result.error = "URL host is empty";
        return result;
    }

    if (path_start != std::string::npos) {
        std::string remainder = url.substr(path_start);
        const std::size_t fragment = remainder.find('#');
        if (fragment != std::string::npos) {
            remainder.erase(fragment);
        }

        if (remainder.empty()) {
            result.path_query = "/";
        } else if (remainder.front() == '?') {
            result.path_query = "/" + remainder;
        } else {
            result.path_query = remainder;
        }
    }

    result.valid = true;
    return result;
}

std::string selected_proxy_hint(bool secure)
{
    const auto env = inspect_environment();

    if (secure && !env.https_proxy.empty()) {
        return env.https_proxy;
    }

    if (!secure && !env.http_proxy.empty()) {
        return env.http_proxy;
    }

    if (!env.https_proxy.empty()) {
        return env.https_proxy;
    }

    if (!env.http_proxy.empty()) {
        return env.http_proxy;
    }

    return {};
}

} // namespace ztxdiag::detail
