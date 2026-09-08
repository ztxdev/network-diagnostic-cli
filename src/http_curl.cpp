#include "ztxdiag/http.hpp"

#include "http_internal.hpp"

#include <curl/curl.h>

#include <chrono>
#include <cmath>
#include <string>

namespace ztxdiag {
namespace {

long long ms_from_seconds(double seconds)
{
    if (seconds < 0.0) {
        return -1;
    }

    return static_cast<long long>(std::llround(seconds * 1000.0));
}

std::string http_version_text(long version)
{
    switch (version) {
#ifdef CURL_HTTP_VERSION_1_0
    case CURL_HTTP_VERSION_1_0:
        return "HTTP/1.0";
#endif
#ifdef CURL_HTTP_VERSION_1_1
    case CURL_HTTP_VERSION_1_1:
        return "HTTP/1.1";
#endif
#ifdef CURL_HTTP_VERSION_2_0
    case CURL_HTTP_VERSION_2_0:
        return "HTTP/2";
#endif
#ifdef CURL_HTTP_VERSION_3
    case CURL_HTTP_VERSION_3:
        return "HTTP/3";
#endif
    default:
        return "unknown";
    }
}

class CurlGlobal {
public:
    CurlGlobal() : ok_(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {}

    ~CurlGlobal()
    {
        if (ok_) {
            curl_global_cleanup();
        }
    }

    [[nodiscard]] bool ok() const noexcept { return ok_; }

private:
    bool ok_{false};
};

} // namespace

HttpResult check_http(
    const std::string& url,
    std::chrono::milliseconds timeout)
{
    HttpResult result;
    result.url = url;

    const auto parsed = detail::parse_http_url(url);
    if (!parsed.valid) {
        result.error = parsed.error;
        return result;
    }

    result.secure = parsed.secure;
    result.scheme = parsed.scheme;
    result.host = parsed.host;
    result.port = parsed.port;
    result.proxy_hint = detail::selected_proxy_hint(parsed.secure);
    result.tls_backend = parsed.secure ? "libcurl/TLS backend" : "not applicable";

    CurlGlobal global;
    if (!global.ok()) {
        result.error = "curl_global_init failed";
        return result;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        result.error = "curl_easy_init failed";
        return result;
    }

    char error_buffer[CURL_ERROR_SIZE]{};

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ztxdiag/0.2");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS,
                     static_cast<long>(timeout.count()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,
                     static_cast<long>(timeout.count()));

    const CURLcode code = curl_easy_perform(curl);

    if (code != CURLE_OK) {
        if (error_buffer[0] != '\0') {
            result.error = error_buffer;
        } else {
            result.error = curl_easy_strerror(code);
        }

        curl_easy_cleanup(curl);
        return result;
    }

    result.success = true;

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);

    char* effective_url = nullptr;
    if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url) == CURLE_OK &&
        effective_url != nullptr) {
        result.effective_url = effective_url;
    } else {
        result.effective_url = url;
    }

    char* primary_ip = nullptr;
    if (curl_easy_getinfo(curl, CURLINFO_PRIMARY_IP, &primary_ip) == CURLE_OK &&
        primary_ip != nullptr) {
        result.remote_address = primary_ip;
    }

    curl_easy_getinfo(curl, CURLINFO_REDIRECT_COUNT, &result.redirect_count);
    result.redirected =
        result.redirect_count > 0 || result.effective_url != result.url;

    long version = 0;
    if (curl_easy_getinfo(curl, CURLINFO_HTTP_VERSION, &version) == CURLE_OK) {
        result.http_version = http_version_text(version);
    }

    double name_lookup = 0.0;
    double connect = 0.0;
    double app_connect = 0.0;
    double start_transfer = 0.0;
    double total = 0.0;

    curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &name_lookup);
    curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect);
    curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &app_connect);
    curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &start_transfer);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);

    result.timing.dns_ms = ms_from_seconds(name_lookup);
    result.timing.tcp_ms =
        ms_from_seconds(connect >= name_lookup ? connect - name_lookup : connect);

    if (parsed.secure && app_connect > 0.0) {
        result.timing.tls_ms =
            ms_from_seconds(app_connect >= connect ? app_connect - connect : app_connect);
    }

    result.timing.ttfb_ms = ms_from_seconds(start_transfer);
    result.timing.total_ms = ms_from_seconds(total);

    curl_easy_cleanup(curl);
    return result;
}

} // namespace ztxdiag
