# 期中：fork client–server（查詢系統資訊）

## 檔案
| 檔案 | 內容 |
|---|---|
| `nsp.h` / `nsp.c` | 共用函式庫 `libnsp.so`：送收訊息、除錯 log |
| `server.c` | 每個連線 fork 一個子行程，回傳系統資訊 |
| `client.c` | 連到 server、送出 `sysinfo` 請求、印出回應 |
| `test10.sh` | 同時 10 個 client 的併發測試 |
| `Makefile` | 編譯 |

## 協定
每則訊息：`[4 位元組資料長度（網路位元組順序）][資料]`
client 送出 `sysinfo`，server 回傳一段系統資訊文字，然後關閉連線。

## 編譯與執行（在 VM 的 midterm 資料夾內）
```
make
./server                    # 視窗 1
./client 127.0.0.1          # 視窗 2
```

## Demo 步驟

### 1. 共用函式庫
```
ldd server client | grep nsp
```

### 2. 同時 10 個 client
```
./server                    # 視窗 1
./test10.sh                 # 視窗 2：看到 1 個父行程 + 10 個子行程、10 條連線
```

### 3. 兩層除錯 log
```
make clean; make DEBUG=1             # 編譯期：把除錯訊息編進去
DEBUG_LOG=1 ./server                 # 執行期：設定環境變數才印出
DEBUG_LOG=1 ./client 127.0.0.1
make clean; make                     # 恢復一般版本
```
只有「DEBUG=1 編譯」且「DEBUG_LOG=1 執行」兩者都成立才會印出 `[DEBUG ...]`。

### 4. 抓封包
```
sudo tcpdump -i lo -nn -w midterm.pcap tcp port 9000     # 視窗 3
./client 127.0.0.1                                        # 視窗 2，完成後 Ctrl+C 停止 tcpdump
tshark -r midterm.pcap
tshark -r midterm.pcap -x                                 # 看每個封包的位元組內容
```
可以看到：三向交握（SYN / SYN-ACK / ACK）→ client 送 4 位元組長度 + `sysinfo` → server 送 4 位元組長度 + 系統資訊 → 四次揮手（FIN）。

### 5. 強健性對照（`./server norobust` 關閉防護）
> 每次重新啟動 server 前，如果出現 `bind: Address already in use`，等 60 秒或改用 `./server`（見防護 3）。

**防護 1：回收子行程（避免殭屍行程）**
```
./server norobust                          # 視窗 1
./client 127.0.0.1; ./client 127.0.0.1     # 視窗 2
ps -o pid,ppid,stat,cmd -C server          # 沒有防護：出現 Z <defunct>
```
改用 `./server` 重做：子行程結束後立刻被回收，沒有 `<defunct>`。

**防護 2：等待請求逾時**
```
./server norobust                          # 視窗 1
./client 127.0.0.1 30                      # 視窗 2：連上後 30 秒不送請求
ps -o pid,stat,cmd -C server               # 沒有防護：子行程一直卡著等
```
改用 `./server` 重做：5 秒後 server 印出 `no valid request (timeout ...)` 並關閉連線。

**防護 3：埠重用（server 重啟）**
```
./server norobust                          # 視窗 1
./client 127.0.0.1                         # 視窗 2
Ctrl+C 停掉 server，立刻再執行 ./server norobust   # 沒有防護：bind: Address already in use
```
改用 `./server` 重做：可以立刻重啟。原因：server 先關閉連線，該連線會在 TIME_WAIT 狀態停留約 60 秒，`SO_REUSEADDR` 允許在這段時間重新綁定同一個埠。
