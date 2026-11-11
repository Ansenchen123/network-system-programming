#!/bin/bash
# 併發測試：同時啟動 10 個 client，每個連上後等 5 秒才送請求。
# 在這 5 秒內列出 server 的行程樹與連線，證明 10 個 client 同時被 10 個子行程服務。
# 用法：先在另一個視窗執行 ./server，再執行 ./test10.sh

for i in 1 2 3 4 5 6 7 8 9 10; do
    ./client 127.0.0.1 5 > /dev/null &
done
sleep 1

echo "=== server 行程樹（1 個父行程 + 10 個子行程）==="
ps -o pid,ppid,stat,cmd --forest -C server

echo "=== port 9000 上已建立的連線 ==="
ss -tn state established '( sport = :9000 )'

wait
echo "=== 10 個 client 全部完成 ==="
