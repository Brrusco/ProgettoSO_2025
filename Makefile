CC=gcc
CFLAGS=-Wall -std=gnu99
INCLUDES=-I./lib

SERVER_SRCS=src/errExit.c src/SHA_256.c src/server.c src/messages.c
CLIENT_SRCS=src/errExit.c src/client.c src/messages.c
BIN_DIR=bin/
OBJ_DIR=obj/

SERVER_OBJS=$(OBJ_DIR)server.o $(OBJ_DIR)errExit.o $(OBJ_DIR)messages.o $(OBJ_DIR)SHA_256.o -lssl -lcrypto -luuid
CLIENT_OBJS=$(OBJ_DIR)client.o $(OBJ_DIR)errExit.o $(OBJ_DIR)messages.o -luuid

all: $(BIN_DIR)server $(BIN_DIR)client

$(BIN_DIR)server: $(SERVER_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

$(BIN_DIR)client: $(CLIENT_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

$(OBJ_DIR)%.o: src/%.c | $(OBJ_DIR)
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

.PHONY: clean

clean:
	@rm -f $(OBJ_DIR)*
	@rm -f $(BIN_DIR)*
	@echo "Removed object files and executables..."
