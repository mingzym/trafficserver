#ifndef IO_USER_THREADS_H
#define IO_USER_THREADS_H

#include <string>
#include <ext/hash_map>
#include <stdint.h>
#include "UF.H"

using namespace std;
namespace std { using namespace __gnu_cxx; }



typedef unsigned long long int TIME_IN_US;



struct UFIOAcceptThreadChooser
{
    virtual std::pair<UFScheduler*,pthread_t> pickThread(int listeningFd) = 0;
    virtual ~UFIOAcceptThreadChooser() {}
};


struct UFIO;
struct UFSleepInfo
{
    UFSleepInfo() { _ufio = 0; }
    UFIO*           _ufio;
};


struct UFIOAcceptArgs
{
    UFIOAcceptArgs() { args = 0; ufio = 0; }
    void* args;
    UFIO* ufio;
};




struct UFIOScheduler;
struct UFIO
{
    friend class UFIOScheduler;
    friend class EpollUFIOScheduler;
    UFIO(UF* uf, int fd = -1);
    ~UFIO();
    bool isSetup(bool makeNonBlocking = true);


    static int setupConnectionToAccept(
                    const char* interface_addr, 
                    unsigned short int port, 
                    unsigned short int backlog = 16000, 
                    bool makeSockNonBlocking = true);
    //the fd must have been created using socket + bind + listen 
    //before passing into this function. It also has to be marked as non-blocking
    //call setupConnectionToAccept first - that will prepare the socket to accept conns
    //the fxn will not return
    //TODO: create ipv6 accept model
    void accept(UFIOAcceptThreadChooser* ufiotChooser,
                unsigned short int ufLocation,
                void* startingArgs,
                void* stackPtr = 0,
                unsigned int stackSize = 0);

    //The fxn will call isSetup which will make the connection request non-blocking and setup the socket
    //TODO: support ipv6 connect
    int connect(const struct sockaddr *addr, int addrlen, TIME_IN_US timeout);

    //since this is most likely going to be run on an edge trigger system
    //reads should be done in such a manner so that if the read call
    //returns back w/ the same number specified, then a following read call
    //should be made to ensure that nothing is left in the network buffer
    //since in edge trigger no more events will be generated for data that was
    //already seen earlier as being in the network. Therefore always make 
    //another call to this fxn if the return was == to nbyte
    ssize_t read(void *buf, size_t nbyte, TIME_IN_US timeout = 0);
    ssize_t write(const void *buf, size_t nbyte, TIME_IN_US timeout = 0);
    int sendto(const char *msg, 
                  int len,
	              const struct sockaddr *to, 
                  int tolen, 
                  TIME_IN_US timeout);
    int sendmsg(const struct msghdr *msg, 
                   int flags,
	               TIME_IN_US timeout);
    int recvfrom(char *buf, 
                    int len, 
                    struct sockaddr *from,
		            int *fromlen, 
                    TIME_IN_US timeout);
    int recvmsg(struct msghdr *msg, 
                   int flags,
	               TIME_IN_US timeout);

    bool close();

    bool setFd(int fd, bool makeNonBlocking = true);


    unsigned int getErrno() const;
    int getFd() const;
    UF* getUF() const;

    const std::string& getRemoteIP() const;
    unsigned int getRemotePort() const;

    UFIOScheduler* getUFIOScheduler() const;
    void setUFIOScheduler(UFIOScheduler* ufios);

    UFSleepInfo*                _sleepInfo;

    static void ufCreateThreadWithIO(pthread_t* tid, std::list<UF*>* ufsToStartWith);
    static UFIOScheduler* getUFIOS();

protected:
    int                         _fd;
    unsigned int                _errno;
    UF*                         _uf;
    UFIOScheduler*              _ufios;

    UFIO() { reset(); }
    void reset();

    std::string                 _remoteIP;
    unsigned int                _remotePort;

    int                         _lastEpollFlag;
    bool                        _amtReadLastTimeEqualToAskedAmt;
};
inline unsigned int UFIO::getErrno() const { return _errno; }
inline int UFIO::getFd() const { return _fd; }
inline UF* UFIO::getUF() const { return _uf; }
inline UFIOScheduler* UFIO::getUFIOScheduler() const { return _ufios; }
inline void UFIO::setUFIOScheduler(UFIOScheduler* ufios) { _ufios = ufios; }
inline const std::string& UFIO::getRemoteIP() const { return _remoteIP; };
inline unsigned int UFIO::getRemotePort() const { return _remotePort; };



struct UFIOScheduler;
//typedef map<pthread_t, UFIOScheduler*> ThreadFiberIOSchedulerMap;
typedef hash_map<pthread_t, UFIOScheduler*, hash<uintptr_t> > ThreadFiberIOSchedulerMap;
struct UFIOScheduler
{
    UFIOScheduler() {}
    virtual ~UFIOScheduler() {}

    virtual bool setupForConnect(UFIO* ufio, TIME_IN_US to = 0) = 0;
    virtual bool setupForAccept(UFIO* ufio, TIME_IN_US to = 0) = 0;
    virtual bool setupForRead(UFIO* ufio, TIME_IN_US to = 0) = 0;
    virtual bool setupForWrite(UFIO* ufio, TIME_IN_US to = 0) = 0;
    virtual bool closeConnection(UFIO* ufio) = 0;

    virtual bool isSetup() { return false; }
    virtual void waitForEvents(TIME_IN_US timeToWait) = 0;

    static ThreadFiberIOSchedulerMap _tfiosscheduler;
};


#define MAX_FDS_FOR_EPOLL 64*1024-1
//typedef map<int, UFIO*> IntUFIOMap;
typedef hash_map<int, UFIO*, hash<int> > IntUFIOMap;
typedef std::multimap<TIME_IN_US, UFSleepInfo*> MapTimeUFIO;
struct EpollUFIOScheduler : public UFIOScheduler
{
    EpollUFIOScheduler(
           UF* uf /*the uf that the scheduler will run on to handle this UFIO*/, 
           unsigned int maxFds = MAX_FDS_FOR_EPOLL);
    ~EpollUFIOScheduler();
    bool isSetup(); //call after the c'tor to verify that the structure is correctly setup

    //inputInfo is the flags info (its an int*) for this addition
    bool setupForConnect(UFIO* ufio, TIME_IN_US to = 0);
    bool setupForAccept(UFIO* ufio, TIME_IN_US to = 0);
    bool setupForRead(UFIO* ufio, TIME_IN_US to = 0);
    bool setupForWrite(UFIO* ufio, TIME_IN_US to = 0);
    bool closeConnection(UFIO* ufio);


    void waitForEvents(TIME_IN_US timeToWait = -1);

    bool                            _interruptedByEventFd;

protected:
    UF*                             _uf;
    int                             _epollFd;
    unsigned int                    _maxFds;
    struct epoll_event*             _epollEventStruct;
    IntUFIOMap                      _intUFIOMap;
    bool                            _alreadySetup;


    MapTimeUFIO                     _sleepList;
    unsigned long long int          _earliestWakeUpFromSleep;

    bool addToScheduler(UFIO* ufio, 
                        void* inputInfo /*flags to identify how ot add*/, 
                        TIME_IN_US to = 0);


    list<UFSleepInfo*>  _availableSleepInfo;
    UFSleepInfo* getSleepInfo();
    void releaseSleepInfo(UFSleepInfo& ufsi);
};

inline UFSleepInfo* EpollUFIOScheduler::getSleepInfo()
{
    if(!_availableSleepInfo.empty())
    {
        UFSleepInfo* ufsi = _availableSleepInfo.front();
        _availableSleepInfo.pop_front();
        return ufsi;
    }

    return new UFSleepInfo();
}

inline void EpollUFIOScheduler::releaseSleepInfo(UFSleepInfo& ufsi)
{
    _availableSleepInfo.push_back(&ufsi);
}

struct IORunner : public UF
{
    void run();
    IORunner(bool registerMe = false)
    {
        if(registerMe)
            _myLoc = UFFactory::getInstance()->registerFunc((UF*)this);
    }
    UF* createUF() { return new IORunner(); }
    static IORunner* _self;
    static int _myLoc;
};

#endif
