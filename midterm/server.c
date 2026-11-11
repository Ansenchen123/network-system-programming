/* server.c：每接到一個 client 連線就 fork 一個子行程服務它，回傳本機系統資訊 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nsp.h"

#define TIMEOUT_SEC 5   /* 防護 2：等待 client 請求的最長秒數 */

/*
 * reap_children：SIGCHLD 的處理函式，子行程結束時由核心呼叫。
 *   用 waitpid 收掉所有已結束的子行程，避免它們變成殭屍行程。
 * 輸入：sig 訊號編號（不使用）
 * 輸出：無
 */
static void reap_children(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

/*
 * build_sysinfo：查詢本機系統資訊，寫成一段文字。
 * 輸入：buf 存放文字的緩衝區；size 緩衝區大小
 * 輸出：回傳寫入的文字長度（不含結尾 '\0'）
 */
static int build_sysinfo(char *buf, int size)
{
    struct utsname u;
    struct sysinfo s;

    uname(&u);
    sysinfo(&s);
    return snprintf(buf, size,
                    "hostname : %s\n"
                    "kernel   : %s %s\n"
                    "uptime   : %ld seconds\n"
                    "load avg : %.2f %.2f %.2f\n"
                    "memory   : %lu MB free / %lu MB total\n"
                    "processes: %d\n"
                    "served by: pid %d\n",
                    u.nodename,
                    u.sysname, u.release,
                    s.uptime,
                    s.loads[0] / 65536.0, s.loads[1] / 65536.0, s.loads[2] / 65536.0,
                    s.freeram * s.mem_unit / 1024 / 1024, s.totalram * s.mem_unit / 1024 / 1024,
                    s.procs,
                    getpid());
}

/*
 * handle_client：子行程服務一個 client：收請求、回應、結束。
 * 輸入：fd 與 client 連線的 socket；robust 1 表示開啟防護，0 表示關閉
 * 輸出：無
 */
static void handle_client(int fd, int robust)
{
    char req[MAX_MSG];
    char reply[MAX_MSG];
    int len;
    struct timeval tv;

    if (robust == 1) {
        /* 防護 2：client 超過 TIMEOUT_SEC 秒沒送請求，recv 就回傳錯誤，子行程不會永遠卡住 */
        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    if (recv_msg(fd, req, MAX_MSG) < 0) {
        printf("[pid %d] no valid request (timeout or disconnected)\n", getpid());
        return;
    }
    log_debug("request: %s", req);
    if (strcmp(req, "sysinfo") == 0) {
        len = build_sysinfo(reply, MAX_MSG);
    } else {
        len = snprintf(reply, MAX_MSG, "unknown request: %s\n", req);
    }
    if (send_msg(fd, reply, len) < 0) {
        printf("[pid %d] send failed (client disconnected)\n", getpid());
        return;
    }
    log_debug("sent %d bytes", len);
}

/*
 * main：流程——建立監聽 socket，每接到一個連線就 fork 一個子行程去服務。
 *   用法：./server            開啟防護機制
 *         ./server norobust   關閉防護機制（示範用）
 */
int main(int argc, char *argv[])
{
    int robust = 1;
    int listen_fd;
    int client_fd;
    int opt = 1;
    pid_t pid;
    struct sockaddr_in addr;
    struct sockaddr_in peer;
    socklen_t peer_len;
    struct sigaction sa;

    if (argc > 1 && strcmp(argv[1], "norobust") == 0) {
        robust = 0;
    }
    /* stdout 改成每行立刻輸出，避免 fork 時子行程複製到還沒印出的內容而重複印 */
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (robust == 1) {
        /* 防護 1：子行程結束時自動回收，避免殭屍行程；SA_RESTART 讓被打斷的 accept 自動重來 */
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = reap_children;
        sa.sa_flags = SA_RESTART;
        sigaction(SIGCHLD, &sa, NULL);
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (robust == 1) {
        /* 防護 3：允許重用還在 TIME_WAIT 狀態的埠，server 重啟時 bind 不會失敗 */
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    listen(listen_fd, 16);
    printf("server pid %d listening on port %d (robust=%d)\n", getpid(), PORT, robust);

    while (1) {
        peer_len = sizeof(peer);
        client_fd = accept(listen_fd, (struct sockaddr *)&peer, &peer_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        pid = fork();
        if (pid == 0) {
            close(listen_fd);
            printf("[pid %d] serving %s:%d\n", getpid(), inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
            handle_client(client_fd, robust);
            close(client_fd);
            printf("[pid %d] done\n", getpid());
            exit(0);
        }
        close(client_fd);
    }
}
