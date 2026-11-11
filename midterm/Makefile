# 用法：make            一般編譯（除錯訊息不編進去）
#       make DEBUG=1    把除錯訊息編進去（切換前先 make clean）
#       make clean      刪除編譯產物

CFLAGS = -Wall

ifdef DEBUG
CFLAGS = -Wall -DDEBUG
endif

all: libnsp.so server client

# 共用函式庫：-fPIC 產生可重定位的程式碼，-shared 輸出成動態函式庫
libnsp.so: nsp.c nsp.h
	gcc $(CFLAGS) -fPIC -shared -o libnsp.so nsp.c

# -L. -lnsp 連結 libnsp.so；-rpath '$$ORIGIN' 讓執行時到執行檔所在的資料夾找 libnsp.so
server: server.c nsp.h libnsp.so
	gcc $(CFLAGS) -o server server.c -L. -lnsp -Wl,-rpath,'$$ORIGIN'

client: client.c nsp.h libnsp.so
	gcc $(CFLAGS) -o client client.c -L. -lnsp -Wl,-rpath,'$$ORIGIN'

clean:
	rm -f libnsp.so server client
