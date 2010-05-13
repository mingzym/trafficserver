CXXFLAGS=-g -O3 -Wall -Wno-deprecated -march=x86-64
CPPFLAGS=-DPIPE_NOT_EFD -I./include
LDFLAGS=-L./lib -lUF -lpthread

.PHONY: all clean lib

all: ufHTTPServer

lib:
	$(MAKE) -C src all

ufHTTPServer:	lib ufHTTPServer.C
	$(CXX) -o $@ $(CPPFLAGS) $(CXXFLAGS) ufHTTPServer.C $(LDFLAGS)

clean: 
	$(MAKE) -C src clean
	$(RM) *.o ufHTTPServer

