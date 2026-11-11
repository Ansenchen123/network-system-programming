/* nsp.h：client 與 server 共用的常數與函式宣告，函式實作在 nsp.c（編成 libnsp.so） */

#define PORT 9000      /* server 監聽的埠號 */
#define MAX_MSG 4096   /* 一則訊息的最大長度（位元組） */

int send_msg(int fd, const char *data, int len);
int recv_msg(int fd, char *buf, int size);
void log_debug(const char *fmt, ...);
