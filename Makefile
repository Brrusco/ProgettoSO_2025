CC=gcc
CFLAGS=-Wall -std=gnu99
INCLUDES=-I./lib

SERVER_SRCS=src/errExit.c src/SHA_256.c src/server.c src/messages.c src/threadOp.c src/semaforo.c
CLIENT_SRCS=src/errExit.c src/client.c src/messages.c

BIN_DIR=bin/
OBJ_DIR=obj/

SERVER_OBJS=$(OBJ_DIR)server.o $(OBJ_DIR)errExit.o $(OBJ_DIR)messages.o $(OBJ_DIR)SHA_256.o $(OBJ_DIR)threadOp.o $(OBJ_DIR)semaforo.o
CLIENT_OBJS=$(OBJ_DIR)client.o $(OBJ_DIR)errExit.o $(OBJ_DIR)messages.o

LIBS=-lssl -lcrypto -luuid

all: $(BIN_DIR)server $(BIN_DIR)client

$(BIN_DIR)server: $(SERVER_OBJS) | $(BIN_DIR)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@ $(LIBS)

$(BIN_DIR)client: $(CLIENT_OBJS) | $(BIN_DIR)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@ -luuid

$(OBJ_DIR)%.o: src/%.c | $(OBJ_DIR)
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

.PHONY: clean start server client all

clean:
	@rm -f $(OBJ_DIR)*
	@rm -f $(BIN_DIR)*
	@echo "Removed object files and executables..."

start:
	clear
	make clean
	make all

server:
	clear
	bin/server

client:
	clear
	bin/client