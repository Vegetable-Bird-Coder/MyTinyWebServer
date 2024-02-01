# MyTinyWebServer

- Linux 下 C++ 轻量级 Web 服务器，支持 HTTP 解析和 GET、POST 方法请求
- 实现基于线程池和非阻塞 socket 的高性能并发服务器架构，集成 epoll 事件处理机制，支持 ET/LT 模式
- 应用 Reactor/Proactor 设计模式优化事件处理流程，引入数据库连接池加速查询
- 设计同步/异步日志系统，实现服务器状态跟踪；构建时间堆定时容器，释放非活动连接
- 服务器经 Webbench 压力测试验证，能够处理 9k 并发连接（Webbench fork 达到 VM 系统瓶颈）