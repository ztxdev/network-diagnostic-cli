# network-diagnostic-cli

**`ztxdiag`** is a small cross-platform C++20 command-line tool for first-line network diagnosis.

It is intended for the situations engineers see repeatedly:

- "The host can be pinged, but the application cannot connect."
- "It works on one PC but not another."
- "DNS resolves differently across machines."
- "Port 443 is unreachable."
- "A proxy or VPN may be changing the path."
- "We need a machine-readable diagnostic result for support or automation."

The V0.1 baseline deliberately focuses on diagnostics that do not require administrator/root privileges.

---

## Current Features

- DNS resolution
- IPv4 / IPv6 address reporting
- TCP connection checks
- TCP connect latency
- Configurable connection timeout
- Local hostname and address inspection
- HTTP / HTTPS proxy environment inspection
- Human-readable text output
- JSON output
- Windows / MSVC support
- Linux / GCC support
- Linux / Clang support
- CTest + GitHub Actions CI

---

## Examples

### One-command diagnostic

```text
ztxdiag github.com 443
```

Example output:

```text
ZTXDIAG
=======

DNS
  OK   140.82.112.4

TCP
  OK   connected
  address: 140.82.112.4
  port: 443
  latency: 42 ms

ENVIRONMENT
  hostname: WORKSTATION
  IPv4 stack: available
  IPv6 stack: available
  HTTP_PROXY: (not set)
  HTTPS_PROXY: http://127.0.0.1:7890
  NO_PROXY: (not set)

RESULT
  PASS
```

### DNS only

```text
ztxdiag dns github.com
```

### TCP / port check

```text
ztxdiag tcp github.com 443
```

With a 2-second timeout:

```text
ztxdiag tcp github.com 443 --timeout 2000
```

### Environment

```text
ztxdiag env
```

### JSON

```text
ztxdiag github.com 443 --json
```

JSON output is suitable for scripts, support tooling, and later integration with monitoring systems.

---

## Exit Codes

| Code | Meaning |
|---:|---|
| 0 | Diagnostic succeeded |
| 2 | Invalid command or arguments |
| 3 | DNS resolution failed |
| 4 | TCP connection failed |

This makes the CLI useful in scripts and CI jobs.

---

## Build on Windows / MSVC

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run:

```powershell
.\build\Release\ztxdiag.exe github.com 443
```

---

## Build on Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/ztxdiag github.com 443
```

---

## Why TCP latency instead of ping?

ICMP echo can be blocked, filtered, rate-limited, or require elevated privileges on some systems.

`ztxdiag` measures the time required to establish the actual TCP connection to the target service.

For application troubleshooting, this is frequently more useful than ICMP reachability alone.

---

## Diagnostic Model

`ztxdiag` treats a connectivity problem as layers:

```text
Name
  ↓
DNS resolution
  ↓
IP address
  ↓
TCP connect
  ↓
Service port
  ↓
Application protocol
```

V0.1 covers the first four layers plus the local proxy/network environment.

---

## Roadmap

### V0.2 — HTTP / HTTPS diagnostics

Planned:

```text
ztxdiag http https://api.github.com
```

Target data:

- HTTP status
- DNS time
- connect time
- TLS time
- first-byte time
- total time
- redirect target
- certificate / TLS error
- proxy usage

HTTP/HTTPS support will use a mature TLS-capable networking library rather than custom TLS code.

### V0.3

- Batch targets
- Output to diagnostic report file
- Config file
- Retry policy
- Optional traceroute/path information
- Release binaries for Windows and Linux

---

## Intended Use

- Remote technical support
- Developer troubleshooting
- Industrial software deployment
- MES / WMS / equipment connectivity diagnosis
- CI connectivity checks
- Customer environment collection
- First-line support automation

---

## Security

`ztxdiag` does not require credentials for the V0.1 commands.

When sharing output publicly, still review:

- hostnames
- internal IP addresses
- proxy addresses
- local machine names

before posting diagnostic data.

---

## Maintainer

**ZTXDev**

C++ · CMake · Linux · Windows · Networking · Industrial Software

Technical diagnostic requests:

https://github.com/ztxdev/software-diagnostic-checklist/issues/new/choose

---

## License

MIT
