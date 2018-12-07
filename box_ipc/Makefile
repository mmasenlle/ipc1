
-include ../../../build/flags.mk

ifeq (,$(NEWKMAKE))

all:
	$(CC) -D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -D__NO_VERSION__ -DKERNELv4 -O \
		-o box_ipc.o -c box_ipc.c -I$(KERNEL_PATH)/include
		
clean:
	rm -f box_ipc.o
	
install:
	cp box_ipc.o $(ROOT_PATH)/libpclit

else

obj-m += box_ipc.o
		
all:
	$(MAKE) -C $(KERNEL_PATH) M=$(shell pwd) modules

clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(shell pwd) clean
	
	
install:
	cp box_ipc.ko $(ROOT_PATH)/libpclit

endif
