# network-diagnostic-cli

`ztxdiag` 是一个 C++20 跨平台网络和 HTTP/HTTPS 诊断命令行工具。

## V0.2 新增

```text
ztxdiag http https://api.github.com
```

现在可以诊断：

- DNS
- TCP
- HTTP / HTTPS
- HTTP 状态码
- 重定向
- HTTP 协议版本
- DNS / TCP / TLS / TTFB / 总时间
- 代理环境提示
- JSON 输出

Windows 使用系统原生 **WinHTTP + Schannel**，不需要另外安装 HTTP/TLS 开发库。

Linux 使用 **libcurl**。

Windows 的同步 WinHTTP 接口不能可靠地单独给出 TLS 握手耗时，因此 V0.2 在 Windows 上将精确 TLS 时间显示为 `n/a`，而不是制造一个看似精确但实际上不可靠的数值。

## Windows 编译

```text
cmake -S . -B build -A x64 -DZTXDIAG_WARNINGS_AS_ERRORS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

测试：

```text
.\build\Release\ztxdiag.exe http https://github.com
.\build\Release\ztxdiag.exe http https://api.github.com --json
```

## 设计原则

这个项目不是“功能越多越好”的网络工具箱。

目标是：

> 用尽量少的命令，把远程技术支持时最有价值的网络证据收集出来。

以后公开分享诊断输出前，要检查并脱敏内部 IP、主机名、代理、客户 URL、Query String、Token 等信息。
