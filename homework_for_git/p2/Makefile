CC=g++
CFLAGS=-c -O1 -Wall -std=c++11
LDFLAGS=
SOURCES=message.cpp messageforsend.cpp messagereceiver.cpp messagesender.cpp client.cpp server.cpp 
ALL_SOURCES = $(SOURCES) clientmain.cpp servermain.cpp
OBJECTS=$(SOURCES:.cpp=.o)

all: $(ALL_SOURCES) chatclt chatsrv
	
chatclt : $(OBJECTS) clientmain.o
	$(CC) $(LDFLAGS) $^ -o $@

chatsrv : $(OBJECTS) servermain.o
	$(CC) $(LDFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean :
	rm -f *.o chatclt chatsrv
