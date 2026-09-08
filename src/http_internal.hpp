#pragma once

#include <cstdint>
#include <string>

namespace ztxdiag::detail {

struct ParsedHttpUrl {
    bool valid{false};
    bool secure{false};
    std::string scheme;
    std::string host;
    std::uint16_t port{0};
    std::string path_query{"/"};
    std::string error;
};

ParsedHttpUrl parse_http_url(const std::string& url);

std::string selected_proxy_hint(bool secure);

} // namespace ztxdiag::detail
