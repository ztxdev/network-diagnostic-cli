# network-diagnostic-cli

[![CI](https://github.com/ztxdev/network-diagnostic-cli/actions/workflows/ci.yml/badge.svg)](https://github.com/ztxdev/network-diagnostic-cli/actions/workflows/ci.yml)

**`ztxdiag`** is a cross-platform C++20 command-line tool for first-line network and HTTP/HTTPS diagnosis.

## V0.2

V0.2 adds application-layer diagnostics:

- DNS resolution
- TCP connectivity and connect latency
- HTTP / HTTPS response checks
- HTTP status
- effective URL / redirect detection
- HTTP protocol version
- DNS / TCP / TLS / TTFB / total timing where the platform backend exposes it
- proxy environment hints
- text and JSON output

### HTTP / HTTPS example

```text
ztxdiag http https://api.github.com
```

Typical Linux output:

```text
HTTP/HTTPS
  URL: https://api.github.com
  OK   response received
  status: 200
  protocol: HTTP/2
  effective URL: https://api.github.com/
  remote address: 140.82.x.x
  redirected: yes
  TLS backend: libcurl/TLS backend
  proxy hint: (not set)

TIMING
  DNS: 8 ms
  TCP: 35 ms
  TLS: 52 ms
  TTFB: 124 ms
  total: 124 ms
```

Windows uses native **WinHTTP + Schannel** and therefore requires no extra HTTP/TLS development package.

Linux uses **libcurl**.

> Windows WinHTTP validates TLS through Schannel, but V0.2 intentionally reports exact standalone TLS-handshake time as `n/a` because the synchronous WinHTTP API path does not expose that metric directly. The tool does not invent timing data.

## Commands

```text
ztxdiag github.com 443
ztxdiag dns github.com
ztxdiag tcp github.com 443 --timeout 2000
ztxdiag http https://api.github.com
ztxdiag http https://api.github.com --json
ztxdiag env
ztxdiag --version
```

## Exit Codes

| Code | Meaning |
|---:|---|
| 0 | Diagnostic transport succeeded |
| 2 | Invalid command or arguments |
| 3 | DNS resolution failed |
| 4 | TCP connection failed |
| 5 | HTTP/HTTPS transport or TLS failed |

An HTTP 404/500 still proves that the HTTP transport path worked, so the command exits 0 when a valid HTTP response is received.

## Build — Windows / MSVC

No additional HTTP library is required.

```powershell
cmake -S . -B build -A x64 -DZTXDIAG_WARNINGS_AS_ERRORS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run:

```powershell
.\build\Release\ztxdiag.exe http https://github.com
```

## Build — Linux

Install libcurl development headers and pkg-config:

```bash
sudo apt-get install pkg-config libcurl4-openssl-dev
```

Build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DZTXDIAG_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/ztxdiag http https://github.com
```

## Backends

| Platform | HTTP/TLS backend |
|---|---|
| Windows | WinHTTP + Schannel |
| Linux | libcurl + system TLS backend |

This avoids custom TLS code.

## Diagnostic model

```text
URL
 ↓
DNS
 ↓
TCP
 ↓
TLS
 ↓
HTTP
 ↓
Status / redirect / timing
```

## Security

Do not publish raw diagnostic output without reviewing:

- internal hostnames
- private IP addresses
- proxy endpoints
- customer URLs
- query strings
- credentials or tokens

Credentials embedded in URLs are rejected.

## Next

V0.3 candidates:

- release binaries
- batch targets
- report files
- configurable redaction
- retry policy
- certificate details
- path / traceroute diagnostics

## Maintainer

**ZTXDev**

C++ · CMake · Linux · Windows · Networking · Industrial Software

Professional diagnostic requests:

https://github.com/ztxdev/software-diagnostic-checklist/issues/new/choose

## License

MIT
