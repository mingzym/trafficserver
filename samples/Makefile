CPP=c++
BUILD_FLAGS=-g -O3 -Wall -DPIPE_NOT_EFD -Wno-deprecated
ARCH=x86-64
INCLUDE=-I../include
INCLUDE_DIR=../include

all:	UFHTTPLoader echoServer

UFHTTPLoader.o:	UFHTTPLoader.C
	$(CPP) $(BUILD_FLAGS) -c -o UFHTTPLoader.o UFHTTPLoader.C $(INCLUDE) -march=$(ARCH)

UFHTTPLoader:	UFHTTPLoader.o
	$(CPP) $(BUILD_FLAGS) -o UFHTTPLoader UFHTTPLoader.o -L../lib/ -lUF -lpthread -march=$(ARCH)

echoServer.o:	echoServer.C
	$(CPP) $(BUILD_FLAGS) -c -o echoServer.o echoServer.C $(INCLUDE) -march=$(ARCH)

echoServer:	echoServer.o
	$(CPP) $(BUILD_FLAGS) -o echoServer echoServer.o -L../lib/ -lUF -lpthread -march=$(ARCH)

clean: 
	rm *.o UFHTTPLoader
