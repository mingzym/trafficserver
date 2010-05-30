CPP=c++
#BUILD_FLAGS=-g -O3 -Wall -Werror -DPIPE_NOT_EFD -Wno-deprecated
#BUILD_FLAGS=-g -Wall -Werror -Wno-deprecated -fno-inline
BUILD_FLAGS=-g -Wall -Werror -Wno-deprecated
ARCH=x86-64
INCLUDE=-I../core/include

#all:	UFHTTPLoader echoServer UFSleepTest UFCondTimedWaitTest httpProxy testSignal testSleep bayeuxCP
all:	UFHTTPLoader echoServer UFCondTimedWaitTest httpProxy testSignal testSleep
#all:	httpProxy UFHTTPLoader
#all:	testProducer testSignal
#all:	testSignal
#all:	UFHTTPLoader

UFHTTPLoader.o:	UFHTTPLoader.C
	$(CPP) $(BUILD_FLAGS) -c -o UFHTTPLoader.o UFHTTPLoader.C $(INCLUDE) -march=$(ARCH)

UFHTTPLoader:	UFHTTPLoader.o
	$(CPP) $(BUILD_FLAGS) -o UFHTTPLoader UFHTTPLoader.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

echoServer.o:	echoServer.C
	$(CPP) $(BUILD_FLAGS) -c -o echoServer.o echoServer.C $(INCLUDE) -march=$(ARCH)

echoServer:	echoServer.o
	$(CPP) $(BUILD_FLAGS) -o echoServer echoServer.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

httpProxy.o:	httpProxy.C
	$(CPP) $(BUILD_FLAGS) -c -o httpProxy.o httpProxy.C $(INCLUDE) -march=$(ARCH)

httpProxy:	httpProxy.o
	$(CPP) $(BUILD_FLAGS) -o httpProxy httpProxy.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

#bayeuxCP.o:	bayeuxCP.C
#	$(CPP) $(BUILD_FLAGS) -c -o bayeuxCP.o bayeuxCP.C $(INCLUDE) -march=$(ARCH)

#bayeuxCP:	bayeuxCP.o
#	$(CPP) $(BUILD_FLAGS) -o bayeuxCP bayeuxCP.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

#UFSleepTest.o:	UFSleepTest.C
#	$(CPP) $(BUILD_FLAGS) -c -o UFSleepTest.o UFSleepTest.C $(INCLUDE) -march=$(ARCH)
#
#UFSleepTest:	UFSleepTest.o
#	$(CPP) $(BUILD_FLAGS) -o UFSleepTest UFSleepTest.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

testProducer.o:	testProducer.C
	$(CPP) $(BUILD_FLAGS) -c -o testProducer.o testProducer.C $(INCLUDE) -march=$(ARCH)

testProducer:	testProducer.o
	$(CPP) $(BUILD_FLAGS) -o testProducer testProducer.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

testSignal.o:	testSignal.C
	$(CPP) $(BUILD_FLAGS) -c -o testSignal.o testSignal.C $(INCLUDE) -march=$(ARCH)

testSignal:	testSignal.o
	$(CPP) $(BUILD_FLAGS) -o testSignal testSignal.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

testSleep.o:	testSleep.C
	$(CPP) $(BUILD_FLAGS) -c -o testSleep.o testSleep.C $(INCLUDE) -march=$(ARCH)

testSleep:	testSleep.o
	$(CPP) $(BUILD_FLAGS) -o testSleep testSleep.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

UFCondTimedWaitTest.o:	UFCondTimedWaitTest.C
	$(CPP) $(BUILD_FLAGS) -c -o UFCondTimedWaitTest.o UFCondTimedWaitTest.C $(INCLUDE) -march=$(ARCH)

UFCondTimedWaitTest:	UFCondTimedWaitTest.o
	$(CPP) $(BUILD_FLAGS) -o UFCondTimedWaitTest UFCondTimedWaitTest.o -L../core/lib/ -lUF -lpthread -march=$(ARCH)

clean: 
	rm *.o UFHTTPLoader
