#include "ztxdiag/diagnostics.hpp"
#include "ztxdiag/format.hpp"

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

    ok &= expect(ztxdiag::parse_port("443", port), "443 must be a valid port");
    ok &= expect(port == 443, "443 must parse to 443");
    ok &= expect(!ztxdiag::parse_port("0", port), "port 0 must be rejected");
    ok &= expect(!ztxdiag::parse_port("65536", port), "port 65536 must be rejected");
    ok &= expect(!ztxdiag::parse_port("abc", port), "non-numeric port must be rejected");

    ok &= expect(
        ztxdiag::json_escape("a\"b\\c\n") == "a\\\"b\\\\c\\n",
        "JSON escaping must handle quotes, backslashes, and newlines");

    const auto localhost = ztxdiag::resolve_dns("localhost");
    ok &= expect(localhost.success, "localhost should resolve");
    ok &= expect(!localhost.addresses.empty(), "localhost must have at least one address");

    const auto env = ztxdiag::inspect_environment();
    ok &= expect(env.ipv4_stack, "IPv4 stack should be available on the CI/runtime host");

    if (!ok) {
        return EXIT_FAILURE;
    }

    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
}
