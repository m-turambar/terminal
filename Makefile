CXX = g++
CC = $(CXX)

CPPFLAGS =-std=c++14\
	-IC:\include

LDFLAGS=-lwsock32\
	-lws2_32

.PHONY: all
all: term

.PHONY: clean
clean:
	del *.o term.exe
	
SRC_FILES = main.cpp
# aqui podrias agregar varios, lineas separadas con \

OBJECTS=$(SRC_FILES:%.cpp=%.o)

term.o:
	

term: $(OBJECTS)
	$(CC) $(INC) $(CPPFLAGS) $(OBJECTS) -o term $(LDFLAGS)