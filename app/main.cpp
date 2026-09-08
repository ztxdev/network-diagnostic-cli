#include "ztxdiag/diagnostics.hpp"
#include "ztxdiag/format.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int exit_ok = 0;
constexpr int exit_usage = 2;
constexpr int exit_dns_failure = 3;
constexpr int exit_tcp_failure = 4;

struct CommonOptions {
    bool json{false};
    std::chrono::milliseconds timeout{3000};
};

void print_usage()
{
    std::cout
        << "ztxdiag - cross-platform network diagnostic CLI\n\n"
        << "Usage:\n"
        << "  ztxdiag <host> <port> [--timeout <ms>] [--json]\n"
        << "  ztxdiag check <host> <port> [--timeout <ms>] [--json]\n"
        << "  ztxdiag dns <host> [--json]\n"
        << "  ztxdiag tcp <host> <port> [--timeout <ms>] [--json]\n"
        << "  ztxdiag env [--json]\n"
        << "  ztxdiag --help\n\n"
        << "Examples:\n"
        << "  ztxdiag github.com 443\n"
        << "  ztxdiag dns github.com\n"
        << "  ztxdiag tcp github.com 443 --timeout 2000\n"
        << "  ztxdiag env --json\n";
}

bool parse_timeout(const std::string& value, std::chrono::milliseconds& timeout)
{
    try {
        std::size_t consumed = 0;
        const long long parsed = std::stoll(value, &consumed, 10);

        if (consumed != value.size() || parsed < 1 || parsed > 600000) {
            return false;
        }

        timeout = std::chrono::milliseconds{parsed};
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_common_options(
    const std::vector<std::string>& args,
    std::size_t start,
    CommonOptions& options,
    std::string& error)
{
    for (std::size_t i = start; i < args.size(); ++i) {
        if (args[i] == "--json") {
            options.json = true;
            continue;
        }

        if (args[i] == "--timeout") {
            if (i + 1 >= args.size()) {
                error = "--timeout requires a value";
                return false;
            }

            if (!parse_timeout(args[++i], options.timeout)) {
                error = "invalid timeout; expected 1..600000 ms";
                return false;
            }
            continue;
        }

        error = "unknown option: " + args[i];
        return false;
    }

    return true;
}

int run_dns(
    const std::string& host,
    const CommonOptions& options)
{
    const auto result = ztxdiag::resolve_dns(host);

    std::cout << (options.json
                      ? ztxdiag::format_dns_json(result)
                      : ztxdiag::format_dns_text(result))
              << '\n';

    return result.success ? exit_ok : exit_dns_failure;
}

int run_tcp(
    const std::string& host,
    const std::string& port_text,
    const CommonOptions& options)
{
    std::uint16_t port = 0;
    if (!ztxdiag::parse_port(port_text, port)) {
        std::cerr << "Invalid port: " << port_text << '\n';
        return exit_usage;
    }

    const auto result = ztxdiag::check_tcp(host, port, options.timeout);

    std::cout << (options.json
                      ? ztxdiag::format_tcp_json(result)
                      : ztxdiag::format_tcp_text(result))
              << '\n';

    return result.success ? exit_ok : exit_tcp_failure;
}

int run_check(
    const std::string& host,
    const std::string& port_text,
    const CommonOptions& options)
{
    std::uint16_t port = 0;
    if (!ztxdiag::parse_port(port_text, port)) {
        std::cerr << "Invalid port: " << port_text << '\n';
        return exit_usage;
    }

    const auto dns = ztxdiag::resolve_dns(host);
    const auto tcp = ztxdiag::check_tcp(host, port, options.timeout);
    const auto env = ztxdiag::inspect_environment();

    std::cout << (options.json
                      ? ztxdiag::format_check_json(dns, tcp, env)
                      : ztxdiag::format_check_text(dns, tcp, env))
              << '\n';

    if (!dns.success) {
        return exit_dns_failure;
    }

    return tcp.success ? exit_ok : exit_tcp_failure;
}

} // namespace

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));

    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        print_usage();
        return args.empty() ? exit_usage : exit_ok;
    }

    CommonOptions options;
    std::string error;

    if (args[0] == "env") {
        if (!parse_common_options(args, 1, options, error)) {
            std::cerr << error << '\n';
            return exit_usage;
        }

        const auto result = ztxdiag::inspect_environment();
        std::cout << (options.json
                          ? ztxdiag::format_environment_json(result)
                          : ztxdiag::format_environment_text(result))
                  << '\n';
        return exit_ok;
    }

    if (args[0] == "dns") {
        if (args.size() < 2) {
            print_usage();
            return exit_usage;
        }

        if (!parse_common_options(args, 2, options, error)) {
            std::cerr << error << '\n';
            return exit_usage;
        }

        return run_dns(args[1], options);
    }

    if (args[0] == "tcp") {
        if (args.size() < 3) {
            print_usage();
            return exit_usage;
        }

        if (!parse_common_options(args, 3, options, error)) {
            std::cerr << error << '\n';
            return exit_usage;
        }

        return run_tcp(args[1], args[2], options);
    }

    if (args[0] == "check") {
        if (args.size() < 3) {
            print_usage();
            return exit_usage;
        }

        if (!parse_common_options(args, 3, options, error)) {
            std::cerr << error << '\n';
            return exit_usage;
        }

        return run_check(args[1], args[2], options);
    }

    // Convenience form: ztxdiag <host> <port> ...
    if (args.size() >= 2 && args[0].rfind("--", 0) != 0) {
        if (!parse_common_options(args, 2, options, error)) {
            std::cerr << error << '\n';
            return exit_usage;
        }

        return run_check(args[0], args[1], options);
    }

    print_usage();
    return exit_usage;
}
