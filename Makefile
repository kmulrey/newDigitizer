# Makefile


CC=arm-unknown-linux-gnu-gcc

LDFLAGS=-static


all: main
clean:
	rm main

main:   Makefile main.c Scope.c Scope.h Socket.c Socket.h Unit.c Unit.h ad_shm.c ad_shm.h
	$(CC) main.c Scope.c Socket.c Unit.c ad_shm.c \
	-I./ -I./include/ -O \
	-lm -ldl -lrt -lpthread -static -o main

