# network-diagnostic-cli

`ztxdiag` 是一个 C++20 跨平台网络诊断命令行工具。

V0.1 的目标不是做一个“功能很多的网络工具箱”，而是解决远程技术支持中最常见的几类问题：

- 域名到底解析到了什么地址？
- TCP 端口到底能不能连接？
- 建立连接大约需要多长时间？
- 当前机器是否配置了 HTTP / HTTPS 代理？
- 为什么同一个服务器一台电脑能连，另一台不能连？
- 如何输出 JSON 交给脚本、CI 或后续诊断平台使用？

## V0.1 功能

- DNS 解析
- IPv4 / IPv6 地址展示
- TCP 端口连接
- TCP Connect 延迟
- 自定义超时
- 本机主机名 / 地址
- HTTP_PROXY / HTTPS_PROXY / NO_PROXY
- 文本输出
- JSON 输出
- Windows / MSVC
- Linux / GCC
- Linux / Clang
- GitHub Actions CI

## 常用命令

完整检查：

```text
ztxdiag github.com 443
```

DNS：

```text
ztxdiag dns github.com
```

TCP：

```text
ztxdiag tcp github.com 443 --timeout 2000
```

环境：

```text
ztxdiag env
```

JSON：

```text
ztxdiag github.com 443 --json
```

## 为什么 V0.1 没有急着自己实现 HTTPS？

HTTPS 不应该为了“看起来功能完整”而自己实现 TLS。

V0.2 会采用成熟的 TLS/HTTP 库，重点输出 DNS、TCP、TLS、TTFB、HTTP Status 等真实诊断指标。

这个项目的原则是：

> 宁可功能少一点，也不要用不可靠的实现制造一个演示品。
