KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
obj-m = remotek.o
remotek-objs = src/remotek/main.o src/remotek/network.o src/remotek/exec.o

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@
	$(RM) $(SERVER_BIN) $(SERVER_TARGET)
