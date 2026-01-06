#!/bin/bash
# multi_telnet.sh

HOST="0.0.0.0"
PORT="8080"
CLIENTS=10  # 并发客户端数量
DURATION=5  # 连接保持时间（秒）

echo "启动 $CLIENTS 个telnet客户端连接到 $HOST:$PORT"

# 启动多个telnet进程
for i in $(seq 1 $CLIENTS); do
    (
        echo "客户端 $i 连接中..."
        # 发送简单的HTTP GET请求
        echo -e "GET / HTTP/1.1\r\nHost: $HOST\r\nConnection: close\r\n\r\n" | \
            timeout $DURATION telnet $HOST $PORT 2>/dev/null | \
            head -5
        sleep 1
        echo "客户端 $i 断开连接"
    ) &
done

# 等待所有后台进程完成
wait
echo "所有客户端测试完成"


GET /631/f25-hw1.html HTTP/2
Host: stevens.netmeister.org
User-Agent: curl/7.64.1
Accept: */*