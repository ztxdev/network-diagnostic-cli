#include "ztxdiag/http.hpp"

#include "http_internal.hpp"
#include "ztxdiag/diagnostics.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <string>
#include <vector>

namespace ztxdiag {
namespace {

class InternetHandle {
public:
    explicit InternetHandle(HINTERNET handle = nullptr) : handle_(handle) {}

    ~InternetHandle()
    {
        if (handle_ != nullptr) {
            WinHttpCloseHandle(handle_);
        }
    }

    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;

    [[nodiscard]] HINTERNET get() const noexcept { return handle_; }

private:
    HINTERNET handle_{nullptr};
};

std::wstring utf8_to_wide(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        value.c_str(), static_cast<int>(value.size()),
        nullptr, 0);

    if (size <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(size), L'\0');

    const int converted = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        value.c_str(), static_cast<int>(value.size()),
        result.data(), size);

    if (converted != size) {
        return {};
    }

    return result;
}

std::string wide_to_utf8(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(
        CP_UTF8, 0,
        value.c_str(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr);

    if (size <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(size), '\0');

    const int converted = WideCharToMultiByte(
        CP_UTF8, 0,
        value.c_str(), static_cast<int>(value.size()),
        result.data(), size, nullptr, nullptr);

    if (converted != size) {
        return {};
    }

    return result;
}

std::string winhttp_error(const char* operation)
{
    return std::string(operation) +
           " failed with WinHTTP error " +
           std::to_string(GetLastError());
}

int timeout_to_int(std::chrono::milliseconds timeout)
{
    const auto count = std::clamp<long long>(
        timeout.count(), 1LL, static_cast<long long>(INT_MAX));
    return static_cast<int>(count);
}

std::string query_header_string(
    HINTERNET request,
    DWORD info_level)
{
    DWORD size = 0;

    WinHttpQueryHeaders(
        request,
        info_level,
        WINHTTP_HEADER_NAME_BY_INDEX,
        WINHTTP_NO_OUTPUT_BUFFER,
        &size,
        WINHTTP_NO_HEADER_INDEX);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return {};
    }

    std::vector<wchar_t> buffer(
        static_cast<std::size_t>(size / sizeof(wchar_t)) + 1U,
        L'\0');

    if (!WinHttpQueryHeaders(
            request,
            info_level,
            WINHTTP_HEADER_NAME_BY_INDEX,
            buffer.data(),
            &size,
            WINHTTP_NO_HEADER_INDEX)) {
        return {};
    }

    return wide_to_utf8(buffer.data());
}

std::string query_effective_url(HINTERNET request)
{
    DWORD size = 0;
    WinHttpQueryOption(request, WINHTTP_OPTION_URL, nullptr, &size);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
        return {};
    }

    std::vector<wchar_t> buffer(
        static_cast<std::size_t>(size / sizeof(wchar_t)) + 1U,
        L'\0');

    if (!WinHttpQueryOption(
            request,
            WINHTTP_OPTION_URL,
            buffer.data(),
            &size)) {
        return {};
    }

    return wide_to_utf8(buffer.data());
}

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
    result.tls_backend =
        parsed.secure ? "Windows WinHTTP / Schannel" : "not applicable";

    const auto dns_started = std::chrono::steady_clock::now();
    const auto dns = resolve_dns(parsed.host);
    const auto dns_ended = std::chrono::steady_clock::now();

    result.timing.dns_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            dns_ended - dns_started)
            .count();

    if (!dns.addresses.empty()) {
        result.remote_address = dns.addresses.front();
    }

    const auto tcp =
        check_tcp(parsed.host, parsed.port, timeout);
    if (tcp.success) {
        result.timing.tcp_ms = tcp.latency.count();
        result.remote_address = tcp.connected_address;
    }

    const std::wstring host = utf8_to_wide(parsed.host);
    const std::wstring path = utf8_to_wide(parsed.path_query);

    if (host.empty() || path.empty()) {
        result.error = "failed to convert URL to UTF-16";
        return result;
    }

    InternetHandle session{
        WinHttpOpen(
            L"ztxdiag/0.2",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0)};

    if (session.get() == nullptr) {
        result.error = winhttp_error("WinHttpOpen");
        return result;
    }

    const int timeout_ms = timeout_to_int(timeout);

    if (!WinHttpSetTimeouts(
            session.get(),
            timeout_ms,
            timeout_ms,
            timeout_ms,
            timeout_ms)) {
        result.error = winhttp_error("WinHttpSetTimeouts");
        return result;
    }

    InternetHandle connection{
        WinHttpConnect(
            session.get(),
            host.c_str(),
            parsed.port,
            0)};

    if (connection.get() == nullptr) {
        result.error = winhttp_error("WinHttpConnect");
        return result;
    }

    const DWORD flags =
        parsed.secure ? WINHTTP_FLAG_SECURE : 0;

    InternetHandle request{
        WinHttpOpenRequest(
            connection.get(),
            L"HEAD",
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags)};

    if (request.get() == nullptr) {
        result.error = winhttp_error("WinHttpOpenRequest");
        return result;
    }

    const auto request_started = std::chrono::steady_clock::now();

    if (!WinHttpSendRequest(
            request.get(),
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0)) {
        result.error = winhttp_error("WinHttpSendRequest");
        return result;
    }

    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        result.error = winhttp_error("WinHttpReceiveResponse");
        return result;
    }

    const auto headers_received = std::chrono::steady_clock::now();

    result.timing.ttfb_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            headers_received - request_started)
            .count();
    result.timing.total_ms = result.timing.ttfb_ms;

    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);

    if (WinHttpQueryHeaders(
            request.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_size,
            WINHTTP_NO_HEADER_INDEX)) {
        result.status_code = static_cast<long>(status_code);
    }

    result.http_version =
        query_header_string(request.get(), WINHTTP_QUERY_VERSION);

    result.effective_url = query_effective_url(request.get());
    if (result.effective_url.empty()) {
        result.effective_url = result.url;
    }

const auto normalize_url = [](std::string value) {
    const std::size_t scheme_end = value.find("://");

    if (scheme_end == std::string::npos) {
        return value;
    }

    const std::size_t authority_start = scheme_end + 3;
    const std::size_t separator =
        value.find_first_of("/?#", authority_start);

    if (separator == std::string::npos) {
        value += '/';
    } else if (value[separator] == '?' || value[separator] == '#') {
        value.insert(separator, "/");
    }

    return value;
};

result.redirected =
    normalize_url(result.effective_url) != normalize_url(result.url);

result.redirect_count = result.redirected ? -1 : 0;

    // WinHTTP performs and validates TLS through Schannel, but it does not
    // expose an exact standalone TLS-handshake duration through this simple
    // synchronous API path. Keep tls_ms as -1 instead of inventing a value.
    result.timing.tls_ms = -1;

    result.success = true;
    return result;
}

} // namespace ztxdiag
