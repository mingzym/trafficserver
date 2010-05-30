ARES_VERSION = 1.7.1
ARES = c-ares-$(ARES_VERSION)
ARESDIR=../ext
ARES_SRC = $(ARESDIR)/$(ARES)/ares.h
ARES_SRC_FILE = $(ARES).tar.gz
ARES_GPG = $(ARES).tar.gz.asc
ARES_LIB = $(ARESDIR)/$(ARES)/.libs/libcares.a 


CXXFLAGS=-g -O3 -Wall -Werror -Wno-deprecated -march=x86-64
#CXXFLAGS=-g -Wall -Werror -Wno-deprecated -march=x86-64 -fno-inline
#CPPFLAGS=-DLOCK_DEBUG -DPIPE_NOT_EFD -I../include -I$(ARESDIR)/$(ARES)
CPPFLAGS=-DPIPE_NOT_EFD -I../include -I$(ARESDIR)/$(ARES)
ARFLAGS=-rv
RANLIB=ranlib
INCLUDE_DIR=../include
LIB_DIR=../lib

.PHONY: all clean

all: $(LIB_DIR)/libUF.a

$(ARES_LIB): $(ARES_SRC)
	pushd $(ARESDIR)/$(ARES) && ./configure && gmake && popd

$(ARES_SRC):
	if [ ! -d $(ARESDIR)/$(ARES) ] ; then \
	   pushd $(ARESDIR) && \
	   wget http://c-ares.haxx.se/$(ARES_SRC_FILE) && \
	   tar xzf ./$(ARES_SRC_FILE) && popd; \
	fi

$(LIB_DIR)/UF.o: UF.C $(INCLUDE_DIR)/UF.H
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UF.o UF.C

$(LIB_DIR)/UFPC.o: UFPC.C $(INCLUDE_DIR)/UFPC.H $(LIB_DIR)/UF.o
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFPC.o UFPC.C

$(LIB_DIR)/UFConnectionPoolImpl.o: UFConnectionPoolImpl.C $(INCLUDE_DIR)/UFConnectionPool.H UFConnectionPoolImpl.H $(LIB_DIR)/UF.o $(LIB_DIR)/UFIO.o
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFConnectionPoolImpl.o UFConnectionPoolImpl.C

$(LIB_DIR)/UFIO.o: UFIO.C $(INCLUDE_DIR)/UFIO.H $(LIB_DIR)/UF.o
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFIO.o UFIO.C

$(LIB_DIR)/UFAres.o: UFAres.C $(INCLUDE_DIR)/UFAres.H $(INCLUDE_DIR)/UFDNS.H $(INCLUDE_DIR)/UFHostEnt.H $(LIB_DIR)/UFIO.o $(LIB_DIR)/UF.o $(ARES_SRC)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFAres.o UFAres.C

$(LIB_DIR)/UFStatSystem.o: UFStatSystem.C $(INCLUDE_DIR)/UFStatSystem.H $(LIB_DIR)/UFIO.o $(LIB_DIR)/UF.o
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFStatSystem.o UFStatSystem.C

$(LIB_DIR)/UFServer.o: UFServer.C $(INCLUDE_DIR)/UFServer.H $(LIB_DIR)/UF.o $(LIB_DIR)/UFIO.o $(LIB_DIR)/UFStatSystem.o $(LIB_DIR)/UFConnectionPoolImpl.o
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $(LIB_DIR)/UFServer.o UFServer.C

$(LIB_DIR)/libUF.a: $(LIB_DIR)/UF.o $(LIB_DIR)/UFPC.o $(LIB_DIR)/UFIO.o $(LIB_DIR)/UFStatSystem.o $(LIB_DIR)/UFServer.o \
	$(LIB_DIR)/UFConnectionPoolImpl.o  $(LIB_DIR)/UFAres.o $(ARES_LIB)
	$(AR) $(ARFLAGS) $(LIB_DIR)/libUF.a $(ARES_LIB) \
	$(LIB_DIR)/UF.o $(LIB_DIR)/UFIO.o $(LIB_DIR)/UFStatSystem.o $(LIB_DIR)/UFServer.o \
	$(LIB_DIR)/UFConnectionPoolImpl.o $(LIB_DIR)/UFAres.o $(LIB_DIR)/UFPC.o
	$(RANLIB) $(LIB_DIR)/libUF.a

clean: 
	echo "cleaning ..."
	$(RM) $(LIB_DIR)/*.o $(LIB_DIR)/*.a

