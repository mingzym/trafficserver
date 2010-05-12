CPP=c++
BUILD_FLAGS=-g -O3 -Wall -DPIPE_NOT_EFD -Wno-deprecated
ARCH=x86-64

all:	ufHTTPServer

ufHTTPServer.o:	ufHTTPServer.C lib/libUF.a
	$(CPP) $(BUILD_FLAGS) -c -I./include -o ufHTTPServer.o ufHTTPServer.C -march=$(ARCH)

ufHTTPServer:	ufHTTPServer.o
	$(CPP) $(BUILD_FLAGS) -o ufHTTPServer ufHTTPServer.o -L./lib -lUF -lpthread -march=$(ARCH)

clean: 
	rm *.o ufHTTPServer
