CC := gcc
PROJ_DIR := .
PROJ_NAME := Driver
OUTPUT_PATH := .

all: server client

server: server.c
	${CC} -o server server.c -pthread

client: client.c
	${CC} -o client client.c -pthread


clean:
	rm -rf server client
