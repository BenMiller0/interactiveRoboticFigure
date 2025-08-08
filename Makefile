Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lasound -lm -lwiringPi

tarosound: taromouth.c
    $(CC) $(CFLAGS) -o tarosound taromouth.c $(LIBS)
