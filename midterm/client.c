/* client.c：連到 server，請求系統資訊並印出 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nsp.h"

/*
 * main：流程——連到 server，等待指定秒數，送出 "sysinfo" 請求，印出回應。
 *   用法：./client <server IP> [等待秒數]
 *   等待秒數：連上後先等幾秒才送請求，用來製造「多個連線同時存在」與「逾時」的情境。
 */
int main(int argc, char *argv[])
{
    int fd;
    int wait_sec = 0;
    struct sockaddr_in addr;
    char reply[MAX_MSG];

    if (argc < 2) {
        printf("usage: %s <server_ip> [wait_seconds]\n", argv[0]);
        return 1;
    }
    if (argc > 2) {
        wait_sec = atoi(argv[2]);
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
        printf("invalid IP address: %s\n", argv[1]);
        return 1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    log_debug("connected to %s:%d, waiting %d seconds", argv[1], PORT, wait_sec);
    sleep(wait_sec);

    if (send_msg(fd, "sysinfo", 7) < 0) {
        printf("send failed (server closed the connection)\n");
        return 1;
    }
    if (recv_msg(fd, reply, MAX_MSG) < 0) {
        printf("no reply (server closed the connection)\n");
        return 1;
    }
    printf("%s", reply);
    close(fd);
    return 0;
}
