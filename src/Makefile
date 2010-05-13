CXXFLAGS=-g -O3 -Wall -Wno-deprecated -march=x86-64
CPPFLAGS=-DPIPE_NOT_EFD -I../include
ARFLAGS=-rv
RANLIB=ranlib
INCLUDE_DIR=../include
LIB_DIR=../lib

.PHONY: all clean

all: libUF.a

UF.o: UF.C $(INCLUDE_DIR)/UF.H
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UF.o UF.C

UFIO.o: UFIO.C $(INCLUDE_DIR)/UFIO.H
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFIO.o UFIO.C

UFStatSystem.o: UFStatSystem.C $(INCLUDE_DIR)/UFStatSystem.H
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFStatSystem.o UFStatSystem.C

UFServer.o: UFServer.C $(INCLUDE_DIR)/UFServer.H
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFServer.o UFServer.C

UFConnectionPoolImpl.o: UFConnectionPoolImpl.C $(INCLUDE_DIR)/UFConnectionPool.H UFConnectionPoolImpl.H
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFConnectionPoolImpl.o UFConnectionPoolImpl.C

libUF.a: UF.o UFIO.o UFStatSystem.o UFServer.o UFConnectionPoolImpl.o
	$(AR) $(ARFLAGS) $(LIB_DIR)/libUF.a \
    $(LIB_DIR)/UF.o $(LIB_DIR)/UFIO.o $(LIB_DIR)/UFStatSystem.o $(LIB_DIR)/UFServer.o $(LIB_DIR)/UFConnectionPoolImpl.o
	$(RANLIB) $(LIB_DIR)/libUF.a

clean: 
	$(RM) $(LIB_DIR)/*.o $(LIB_DIR)/*.a

