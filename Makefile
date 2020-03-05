obj-m := rfs.o
rfs-objs := krfs.o super.o inode.o dir.o file.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

CFLAGS_krfs.o := -DDEBUG
CFLAGS_super.o := -DDEBUG
CFLAGS_inode.o := -DDEBUG
CFLAGS_dir.o := -DDEBUG
CFLAGS_file.o := -DDEBUG

all: ko mkfs-rfs

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mkfs-rfs_SOURCES:
	mkfs-rfs.c rfs.h

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm mkfs-rfs
