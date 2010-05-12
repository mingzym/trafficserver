CPP=c++
BUILD_FLAGS=-g -O3 -Wall -DPIPE_NOT_EFD -Wno-deprecated -m32
ARCH=x86-64
AR=ar -rv
RANLIB=ranlib
INCLUDE_DIR=../include
INCLUDE=-I../include
LIB=../lib

all:	libUF.a


UF.o:	UF.C $(INCLUDE_DIR)/UF.H
	$(CPP) $(BUILD_FLAGS) -c -o $(LIB)/UF.o UF.C -I../include -march=$(ARCH) 

UFIO.o:	UFIO.C $(INCLUDE_DIR)/UFIO.H
	$(CPP) $(BUILD_FLAGS) -c -o $(LIB)/UFIO.o UFIO.C $(INCLUDE) -march=$(ARCH)

UFStatSystem.o: UFStatSystem.C $(INCLUDE_DIR)/UFStatSystem.H
	$(CPP) $(BUILD_FLAGS) -c -o $(LIB)/UFStatSystem.o UFStatSystem.C $(INCLUDE) -march=$(ARCH)

UFServer.o: UFServer.C $(INCLUDE_DIR)/UFServer.H
	$(CPP) $(BUILD_FLAGS) -c -o $(LIB)/UFServer.o UFServer.C $(INCLUDE) -march=$(ARCH)

UFConnectionPoolImpl.o: UFConnectionPoolImpl.C $(INCLUDE_DIR)/UFConnectionPool.H UFConnectionPoolImpl.H
	$(CPP) $(BUILD_FLAGS) -c -o $(LIB)/UFConnectionPoolImpl.o UFConnectionPoolImpl.C $(INCLUDE) -march=$(ARCH)

libUF.a:	UF.o UFIO.o UFStatSystem.o UFServer.o UFConnectionPoolImpl.o
	$(AR) $(LIB)/libUF.a $(LIB)/UF.o $(LIB)/UFIO.o $(LIB)/UFStatSystem.o $(LIB)/UFServer.o $(LIB)/UFConnectionPoolImpl.o
	$(RANLIB) $(LIB)/libUF.a

clean: 
	rm *.o *.a
