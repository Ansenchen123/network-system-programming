/* nsp.c：client 與 server 共用的工具函式，編譯成動態函式庫 libnsp.so */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "nsp.h"

/*
 * write_all：把 buf 的 len 個位元組全部送出。
 *   send() 一次可能只送出一部分，所以用迴圈送到全部送完為止。
 *   MSG_NOSIGNAL：對方已斷線時回傳錯誤，而不是收到 SIGPIPE 讓整個行程被殺掉。
 * 輸入：fd 已連線的 socket；buf 要送的資料；len 要送的位元組數
 * 輸出：全部送完回傳 0；失敗回傳 -1
 */
static int write_all(int fd, const char *buf, int len)
{
    int done = 0;
    int n;

    while (done < len) {
        n = send(fd, buf + done, len - done, MSG_NOSIGNAL);
        if (n <= 0) {
            return -1;
        }
        done = done + n;
    }
    return 0;
}

/*
 * read_all：從 fd 讀滿 len 個位元組到 buf。
 *   recv() 一次可能只讀到一部分，所以用迴圈讀到讀滿為止。
 * 輸入：fd 已連線的 socket；buf 存放資料的緩衝區；len 要讀的位元組數
 * 輸出：讀滿回傳 0；對方斷線、逾時或出錯回傳 -1
 */
static int read_all(int fd, char *buf, int len)
{
    int done = 0;
    int n;

    while (done < len) {
        n = recv(fd, buf + done, len - done, 0);
        if (n <= 0) {
            return -1;
        }
        done = done + n;
    }
    return 0;
}

/*
 * send_msg：送出一則訊息。
 *   訊息格式：[4 位元組資料長度（網路位元組順序）][資料]
 * 輸入：fd 已連線的 socket；data 要送的資料；len 資料長度
 * 輸出：成功回傳 0；失敗回傳 -1
 */
int send_msg(int fd, const char *data, int len)
{
    uint32_t net_len = htonl(len);

    if (write_all(fd, (const char *)&net_len, 4) < 0) {
        return -1;
    }
    return write_all(fd, data, len);
}

/*
 * recv_msg：收一則 send_msg 送來的訊息，並在資料結尾補 '\0'，方便當字串使用。
 * 輸入：fd 已連線的 socket；buf 存放資料的緩衝區；size 緩衝區大小
 * 輸出：成功回傳資料長度；對方斷線、逾時、或資料長度超過緩衝區回傳 -1
 */
int recv_msg(int fd, char *buf, int size)
{
    uint32_t net_len;
    int len;

    if (read_all(fd, (char *)&net_len, 4) < 0) {
        return -1;
    }
    len = ntohl(net_len);
    if (len < 0 || len >= size) {
        return -1;
    }
    if (read_all(fd, buf, len) < 0) {
        return -1;
    }
    buf[len] = '\0';
    return len;
}

/*
 * log_debug：把除錯訊息印到 stderr，用法和 printf 相同。
 *   編譯期開關：用 make DEBUG=1 編譯時函式才有內容，否則是空函式，什麼都不印。
 *   執行期開關：執行時設定環境變數 DEBUG_LOG=1 才會真的印出。
 * 輸入：fmt 格式字串；... 對應 fmt 的參數
 * 輸出：無
 */
void log_debug(const char *fmt, ...)
{
#ifdef DEBUG
    va_list args;
    char *env = getenv("DEBUG_LOG");

    if (env == NULL || strcmp(env, "1") != 0) {
        return;
    }
    fprintf(stderr, "[DEBUG pid %d] ", getpid());
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
#endif
}
