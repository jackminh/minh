/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/include/
https://www.man7.org/linux/man-pages/man3/sockaddr.3type.html

https://man.archlinux.org/man/memcpy.3.en (需要梯子)

https://insecure.org/stf/smashstack.html


      +---------------------------+ <-- high address
        | command-line arguments    |
        | and environment variables |
        +---------------------------+
        |           stack           |
        +- - - - - - - - - - - - - -+
        |             |             |
        |             |             |
        |             V             |
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        +---------------------------+
        |    shared memory area     |
        +---------------------------+
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        |                           |
        |             ^             |
        |             |             |
        |             |             |
        +- - - - - - - - - - - - - -+
        |            heap           |
        +---------------------------+
        | uninitialized data or bss |
        | (Block Started by Symbol) |
        +---------------------------+
        |      initialized data     |
        +---------------------------+
        |        text segment       |
        +---------------------------+ <-- low address


c程序不是从main开始执行
_start()-->__start()-->exit(main())

先执行 _start函数,再执行__start函数,最后执行exit函数
main函数执行完后是将返回值给到exit函数

操作系统加载程序
    ↓
_start (汇编入口)
    ↓
__start (C运行时初始化)
    ↓
__libc_start_main (Glibc)
    ↓
初始化堆、栈、全局变量
    ↓
调用 main()
    ↓
main() 执行
    ↓
return 到 __libc_start_main
    ↓
exit(main的返回值)
    ↓
程序结束





echo -ne "\x10\x1c\x00\x04MQTT\x04\xc2\x00\x3c\x00\x07client1\x00\x05will/\x00\x07goodbye" | telnet 0.0.0. 9527


分块传输的报文格式
// 分块传输的完整结构
[块大小（十六进制）]\r\n
[块数据]\r\n
[块大小]\r\n
[块数据]\r\n
...
0\r\n                    // 最后一块，大小为0
[尾部头部（可选）]\r\n
\r\n                    // 结束


GET /api/users HTTP/1.1
Host: api.example.com
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
Accept: application/json
Accept-Language: en-US,en;q=0.9
Accept-Encoding: gzip, deflate
Connection: keep-alive
Cache-Control: no-cache

GET /api/users?page=1&limit=10&sort=name&order=asc HTTP/1.1
Host: api.example.com
User-Agent: MyApp/1.0
Accept: application/json
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c



POST /api/login HTTP/1.1
Host: www.example.com
Content-Type: application/x-www-form-urlencoded
Content-Length: 29
User-Agent: Mozilla/5.0
Accept: text/html,application/xhtml+xml

username=johndoe&password=secret123


POST /api/users HTTP/1.1
Host: api.example.com
Content-Type: application/json
Content-Length: 58
Authorization: Bearer token123
User-Agent: MyClient/1.0
Accept: application/json

{
  "name": "John Doe",
  "email": "john@example.com",
  "age": 30
}



POST /api/upload HTTP/1.1
Host: upload.example.com
Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Length: 428
User-Agent: Chrome/91.0

------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="file"; filename="example.jpg"
Content-Type: image/jpeg

[binary image data here...]
------WebKitFormBoundary7MA4YWxkTrZu0gW
Content-Disposition: form-data; name="description"

A beautiful sunset photo
------WebKitFormBoundary7MA4YWxkTrZu0gW--


POST /api/cart/add HTTP/1.1
Host: shop.example.com
Content-Type: application/json
Content-Length: 43
Cookie: session_id=abc123; user_pref=dark_mode
User-Agent: Mozilla/5.0
Accept: */*

{"product_id": 456, "quantity": 2, "color": "blue"}





PUT /api/users/123 HTTP/1.1
Host: api.example.com
Content-Type: application/json
Content-Length: 78
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c
If-Match: "686897696a7c876b7e"

{
  "id": 123,
  "name": "John Smith",
  "email": "john.smith@example.com",
  "status": "active"
}



PUT /api/articles/789 HTTP/1.1
Host: blog.example.com
Content-Type: application/json
Content-Length: 156
If-Unmodified-Since: Sat, 29 Oct 2022 19:43:31 GMT
If-Match: "33a64df551425fcc55e4d42a148795d9"

{
  "title": "Updated Article Title",
  "content": "This is the updated content of the article...",
  "tags": ["technology", "programming"],
  "published": true
}



DELETE /api/users/456 HTTP/1.1
Host: api.example.com
Authorization: Bearer admin_token_123
User-Agent: AdminClient/2.0
Accept: application/json


-------------------------------------------------
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13
Sec-WebSocket-Protocol: chat, superchat
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
Origin: http://example.com


如果出现以下情况，呼叫将失败：
[EAGAIN] 套接字被标记为非阻塞，而接收操作将会阻塞，或者已设置接收超时，且在收到数据之前超时已过期。
[EBADF] 参数 socket 是无效的描述符。
[ECONNRESET] 对方在套接字接收尝试期间关闭了连接。
[EFAULT] 接收缓冲区指针指向了进程地址空间之外。
[EINTR] 接收操作因在有可用数据之前收到信号而中断。
[EINVAL] MSG_OOB 已设置，但没有带外数据可用。
[ENOBUFS] 内存缓冲区分配尝试失败。
[ENOTCONN] 套接字与面向连接的协议相关联，但尚未建立连接（请参阅 connect(2)、connectx(2) 和 accept(2)）。
[ENOTSOCK] 该参数 socket 并非指向套接字。
[EOPNOTSUPP] 该套接字的类型和/或协议不支持在标志中指定的选项。
[超时] 连接超时。
recvfrom() 调用也可能在以下情况失败：
[EINVAL] iov_len 值的总和超过了 ssize_t 类型所能表示的范围。
recvmsg() 调用还可能在以下情况失败：
[EMSGSIZE] 消息所指向的 msghdr 结构中的 msg_iovlen 成员小于或等于 0，或者大于 IOV_MAX。
[ENOMEM] 可用内存不足。



1. 常用信号及键盘映射
终端控制信号
信号    编号  默认动作    键盘快捷键       描述
SIGINT  2   Terminate   Ctrl+C        中断进程
SIGQUIT 3   Core        Ctrl+\        退出进程（产生core）
SIGTSTP 20  Stop        Ctrl+Z        暂停（挂起）进程
SIGCONT 18  Continue    （无默认按键）  继续执行暂停的进程
SIGHUP  1   Terminate   （终端关闭时）  挂起信号



if(pid == 0){
    printLog("%d > 设置工作进程名称\n", getpid());
    //设置工作进程名
    set_process_name_fmt("worker-%d", i);
    //子进程
    printLog("%d > 工作进程 %d 启动\n", getpid(), i); 
    //重置信号处理（子进程不需要处理这些信号）
    signal(SIGCHLD, SIG_DFL);
    signal(SIGINT,  SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    // 执行服务器函数
    func(home);
    // 退出子进程
    exit(0);
} else {
    // 父进程记录子进程ID
    worker_pids[i] = pid;
    printLog("%d > 创建工作进程 %d (PID: %d)\n", getpid(), i, pid);
    usleep(100000);  // 短暂延迟，避免同时创建太多进程
}
以上子进程只重置这三个信号的原因
1. SIGCHLD（子进程退出信号）
signal(SIGCHLD, SIG_DFL);  // 恢复默认处理
原因：
父进程已经设置了 SIGCHLD 处理器来监控和重启子进程
子进程不应该处理 SIGCHLD，因为子进程可能也会创建自己的子进程
如果子进程继承了父进程的 SIGCHLD 处理器，当子进程自己的子进程退出时，会错误地调用父进程的信号处理器
默认行为：忽略（不执行任何操作），这正是我们想要的

2. SIGINT（Ctrl+C 中断信号）
signal(SIGINT, SIG_DFL);  // 恢复默认处理
原因：
父进程捕获 SIGINT 以实现优雅关闭（通知所有子进程，等待退出）
子进程应该由父进程控制关闭，而不是直接响应 Ctrl+C
如果用户在终端按 Ctrl+C：
父进程收到信号 → 发送 SIGTERM 给所有子进程 → 子进程优雅关闭
如果子进程也捕获 SIGINT，会直接退出，父进程无法统一管理

3. SIGTERM（终止信号）
signal(SIGTERM, SIG_DFL);  // 恢复默认处理
原因：
子进程应该接受父进程的终止命令
父进程在优雅关闭时会给所有子进程发送 SIGTERM
如果子进程忽略了 SIGTERM，就无法被正常关闭


总结：主进程应该处理的信号
信号     必须处理    用途             建议操作
SIGCHLD ✅ 必须    监控工作进程状态    重启崩溃的工作进程
SIGINT  ✅ 必须    Ctrl+C 中断     优雅关闭所有进程
SIGTERM ✅ 必须    kill 默认信号    优雅关闭所有进程
SIGPIPE ✅ 必须    管道破裂         SIG_IGN（忽略）
SIGHUP  ✅ 推荐    配置重载         重新读取配置文件
SIGUSR1 ✅ 推荐    用户自定义1      状态转储/调试
SIGUSR2 ✅ 推荐    用户自定义2      日志控制/管理
SIGQUIT ⚠️ 可选   快速关闭          产生core dump
SIGWINCH ⚠️ 可选   窗口大小变化     动态调整输出


