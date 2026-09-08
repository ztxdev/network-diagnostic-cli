#pragma once

#include "ztxdiag/diagnostics.hpp"
#include "ztxdiag/http.hpp"

#include <string>

namespace ztxdiag {

std::string json_escape(const std::string& value);

std::string format_dns_text(const DnsResult& result);
std::string format_dns_json(const DnsResult& result);

std::string format_tcp_text(const TcpResult& result);
std::string format_tcp_json(const TcpResult& result);

std::string format_environment_text(const EnvironmentResult& result);
std::string format_environment_json(const EnvironmentResult& result);

std::string format_http_text(const HttpResult& result);
std::string format_http_json(const HttpResult& result);

std::string format_check_text(
    const DnsResult& dns,
    const TcpResult& tcp,
    const EnvironmentResult& env);

std::string format_check_json(
    const DnsResult& dns,
    const TcpResult& tcp,
    const EnvironmentResult& env);

} // namespace ztxdiag
