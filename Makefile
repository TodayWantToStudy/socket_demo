CC = g++
CFLAGS = -std=c++11
SRC_DIR = src/
BUILD_DIR = build/
RELEASE_DIR = bin/

all:Server.o Client.o
	$(CC) $(CFLAGS) $(SRC_DIR)ServerMain.cpp $(BUILD_DIR)Server.o -o $(RELEASE_DIR)chatroom_server	
	$(CC) $(CFLAGS) $(SRC_DIR)ClientMain.cpp $(BUILD_DIR)Client.o -o $(RELEASE_DIR)chatroom_client

Server.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)Server.cpp -o $(BUILD_DIR)Server.o

Client.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)Client.cpp	-o $(BUILD_DIR)Client.o

clean: 
	rm -f build/*.o bin/*