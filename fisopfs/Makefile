CC = gcc
CFLAGS := -ggdb3 -O2 -Wall -std=c11
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -Wno-unused-function -Wvla

# Flags for FUSE
LDLIBS := $(shell pkg-config fuse --cflags --libs)

# Name for the filesystem!
FS_NAME := fisopfs

$(FS_NAME): fs.o

all: build
	
build: $(FS_NAME)

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

format: .clang-format
	clang-format -i fs.c fisopfs.c tester.h tests.c

docker-build:
	./dock build

docker-run:
	./dock run

docker-exec:
	./dock exec

clean:
	rm -rf $(EXEC) *.o core vgcore.* $(FS_NAME)

test: build
	$(CC) $(CFLAGS) -DFS_DEBUG=0 fs.c tests.c -o tests
	./tests
.PHONY: all build clean format docker-build docker-run docker-exec

# ./fisopfs -f pruebas --filedisk persisnce_file.fisopfs