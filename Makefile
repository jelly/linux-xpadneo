obj-m += hid-microsoft-xbox.o

SRC := $(shell pwd)
KVER=$(shell uname -r)
KDIR=/lib/modules/$(KVER)/build

all:
	make -C $(KDIR) M=$(SRC)

modules_install:
	make -C $(KDIR) M=$(SRC) modules_install

compile_commands.json: $(SRC)
	bear -- make -n -B
