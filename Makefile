KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
obj-m = remotek.o
remotek-objs = src/remotek/main.o src/remotek/network.o src/remotek/exec.o

SERVER_SRC = src/server/server.c
SERVER_BIN = $(SERVER_SRC:.c=)
SERVER_TARGET = server

modules: server
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@
	$(RM) $(SERVER_BIN) $(SERVER_TARGET)

server:
	$(MAKE) -C src/server/
	@mv $(SERVER_BIN) $(SERVER_TARGET)
