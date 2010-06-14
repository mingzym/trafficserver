#include "UFIO.H"
#include "UFConnectionPool.H"
#include "UFStatSystem.H"
#include "UFStats.H"
#include <netdb.h>
#include <sys/socket.h> 
#include <sys/time.h> 

#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <string.h>
using namespace std;

static int makeSocketNonBlocking(int fd)
{
    int flags = 1;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0 ||
        fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;

    return fd;
}

void UFIO::reset()
{
    _fd = -1;
    _uf = 0;
    _errno = 0; 
    _ufios = 0; 
    _lastEpollFlag = 0;
    _sleepInfo = 0;
    _markedActive = false;
    _active = true;
}

UFIO::~UFIO()
{
    close();
}

UFIO::UFIO(UF* uf, int fd)
{ 
    reset();
    _uf = (uf) ? uf : UFScheduler::getUF(pthread_self());

    if(fd != -1)
        setFd(fd);
}

bool UFIO::close()
{
    if(_ufios)
        _ufios->closeConnection(this);
    _ufios = 0;

    if(_fd != -1)
        ::close(_fd);
    _fd = -1;

    return true;
}

bool UFIO::setFd(int fd, bool makeNonBlocking)
{
    if(_fd != -1)
        close();

    _fd = fd;
    if(makeNonBlocking)
        return ((makeSocketNonBlocking(_fd) != -1) ? true : false);
    return true;
}

bool UFIO::isSetup(bool makeNonBlocking)
{
    if(_fd != -1)
        return true;

    if ((_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr<<"couldnt setup socket "<<strerror(errno)<<endl;
        return false;
    } 

    if(makeNonBlocking)
        return ((makeSocketNonBlocking(_fd) != -1) ? true : false);
    return true;
}

int UFIO::setupConnectionToAccept(const char* i_a, 
                                  unsigned short int port, 
                                  unsigned short int backlog,
                                  bool makeSockNonBlocking)
{
    int fd = -1;
    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr<<"couldnt setup socket "<<strerror(errno)<<endl;
        return false;
    } 

    if(makeSockNonBlocking && (makeSocketNonBlocking(fd) == -1))
    {
        ::close(fd);
        return -1;
    }

    const char* interface_addr = ((i_a) && strlen(i_a)) ? i_a : "0.0.0.0";

    int n = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&n, sizeof(n)) < 0) 
    {
        cerr<<"couldnt setup reuseaddr for accept connection"<<endl;
        errno = EINVAL;
        ::close(fd);
        return -1;
    }
   
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(interface_addr);
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) //interface given as a name
    {
        struct hostent *hp;
        if ((hp = gethostbyname(interface_addr)) == NULL)
        {
            cerr<<"couldnt resolve name "<<strerror(errno)<<endl;
            ::close(fd);
            errno = EINVAL;
            return -1;
        }
        memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
    }

    if (bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        cerr<<"couldnt bind to "<<interface_addr<<" on port "<<port<<" - "<<strerror(errno)<<endl;

        ::close(fd);
        errno = EINVAL;
        return -1;
    }

    if (listen(fd, backlog) != 0)
    {
        cerr<<"couldnt setup listen to "<<interface_addr<<" on port "<<port<<" - "<<strerror(errno)<<endl;
        ::close(fd);
        errno = EINVAL;
        return false;
    }

    return fd;
}

void UFIO::accept(UFIOAcceptThreadChooser* ufiotChooser,
                  unsigned short int ufLocation,
                  void* startingArgs,
                  void* stackPtr,
                  unsigned int stackSize)
{
    if(!_uf)
    {
        cerr<<"no user fiber associated with accept request"<<endl;
        return;
    }
    if(!ufiotChooser)
    {
        cerr<<"have to provide a fxn to pick the thread to assign the new task to"<<endl;
        return;
    }

    //setup the UFIOScheduler* for this UFIO
    UFIOScheduler* tmpUfios = _ufios;
    if(!tmpUfios)
    {
        //find the ufios for this thread - this map operation should only be done once
        ThreadFiberIOSchedulerMap::iterator index = UFIOScheduler::_tfiosscheduler.find(pthread_self());
        if(index != UFIOScheduler::_tfiosscheduler.end())
            tmpUfios = index->second;
        else
        {
            cerr<<"couldnt find thread io scheduler for thread "<<pthread_self()<<" - please create one first and assign to the thread - current size of that info = "<<UFIOScheduler::_tfiosscheduler.size()<<endl;
            exit(1); //TODO: may not be necessary to exit here
        }
    }

    int acceptFd = 0;
    struct sockaddr_in cli_addr;
    int sizeof_cli_addr = sizeof(cli_addr);
    UFScheduler* ufs = 0;
    pthread_t tToAddTo = 0;
    pair<UFScheduler*, pthread_t> result;
    bool breakFromMainLoop = false;
    list<UF*> listOfUFsToAdd;
    unsigned int listOfUFsToAddSize = 0;
    while(!breakFromMainLoop)
    {
        //add to the scheduler to see if there was any read activity on it
        //also sets the _ufios of this object 
        if(!tmpUfios->setupForAccept(this))
        {
            cerr<<"couldnt setup for accept - "<<strerror(errno)<<endl;
            exit(1);
        }

        while(1)
        {
            errno = 0;
            acceptFd = ::accept(_fd, (struct sockaddr *)&cli_addr, (socklen_t*)&sizeof_cli_addr);
            if(acceptFd == 0) //hit the timeout
                break;
            else if(acceptFd > 0) { } //handled below
            else if(acceptFd < 0)
            {
                if(errno == EINTR)
                    continue; //try to re-read
                else if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    break; //have to go back and wait for activity
                else
                {
                    _errno = errno;
                    cerr<<"error on accept call = "<<strerror(errno)<<endl;
                    exit(1); //TODO: re-evaluate exit (could be a breakfrommainloop)
                }
            }

            //make the new socket non-blocking
            if(makeSocketNonBlocking(acceptFd) < 1)
            {
                cerr<<"couldnt make accepted socket "<<acceptFd<<" non-blocking"<<strerror(errno)<<endl;
                ::close(acceptFd);
                _errno = errno;
                continue;
            }


            //pass the new socket created to the UF that can deal w/ the request
            UFIOAcceptArgs* connectedArgs = new UFIOAcceptArgs();
            connectedArgs->args = startingArgs;
            //create the UF to handle the new fd
            UF* uf = UFFactory::getInstance()->selectUF(ufLocation)->createUF();
            if(!uf)
            {
                cerr<<"couldnt create new user fiber after accepting conns"<<endl;
                exit(1); //TODO: check if this is necessary
            }
            connectedArgs->ufio = new UFIO(uf, acceptFd);
            if(!connectedArgs->ufio)
            {
                cerr<<"couldnt create UFIOAcceptArgs"<<endl;
                exit(1);
            }
            connectedArgs->ufio->_remoteIP = inet_ntoa(cli_addr.sin_addr);
            connectedArgs->ufio->_remotePort = cli_addr.sin_port;
            uf->_startingArgs = connectedArgs;

            listOfUFsToAdd.push_back(uf);
            listOfUFsToAddSize++;

            if(listOfUFsToAddSize == 100)
            {
                //add fiber to the thread that is recommended's scheduler
                result = ufiotChooser->pickThread(_fd);
                ufs = result.first;
                tToAddTo = result.second;
                if(!ufs || !ufs->addFiberToScheduler(listOfUFsToAdd, tToAddTo))
                {
                    cerr<<"couldnt find thread to assign "<<acceptFd<<" or couldnt add fiber to scheduler"<<endl;
                    exit(1);
                }

                listOfUFsToAdd.clear();
                listOfUFsToAddSize = 0;
            }
        }

        //add the remaining userfibers from the last iteration
        if(listOfUFsToAddSize)
        {
            //add fiber to the thread that is recommended's scheduler
            result = ufiotChooser->pickThread(_fd);
            ufs = result.first;
            tToAddTo = result.second;
            if(!ufs || !ufs->addFiberToScheduler(listOfUFsToAdd, tToAddTo))
            {
                cerr<<"couldnt find thread to assign "<<acceptFd<<" or couldnt add fiber to scheduler"<<endl;
                exit(1);
            }
            listOfUFsToAdd.clear();
            listOfUFsToAddSize = 0;
        }
    }
}

UFIOScheduler* UFIOScheduler::getUFIOS(pthread_t tid)
{
    UFIOScheduler* tmpUfios = 0;

    if(!tid || tid == pthread_self())
        return (UFIOScheduler*)pthread_getspecific(_keyToIdentifySchedulerOnThread);


    //find the ufios for this thread - this map operation should only be done once
    ThreadFiberIOSchedulerMap::iterator index = UFIOScheduler::_tfiosscheduler.find(pthread_self());
    if(index != UFIOScheduler::_tfiosscheduler.end())
        tmpUfios = index->second;
    else
    {
        cerr<<"couldnt find thread io scheduler for thread "<<pthread_self()<<" - please create one first and assign to the thread - current size of that info = "<<UFIOScheduler::_tfiosscheduler.size()<<endl;
        exit(1); //TODO: may not be necessary to exit here
    }

    return tmpUfios;
}

ssize_t UFIO::read(void *buf, size_t nbyte, TIME_IN_US timeout)
{
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    ssize_t n = 0;;
    while(1)
    {
        n = ::read(_fd, buf, nbyte);
        if(n > 0) 
        {
            UFStatSystem::increment(UFStats::bytesRead, n);
            return n;
        }
        else if(n < 0)
        {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                _errno = 0;
                _markedActive = false;
                while(!_markedActive) 
                {
                    //wait for something to read first
                    if(!tmpUfios->setupForRead(this, timeout))
                        return -1;
                }
                continue;
            }
            else if(errno == EINTR)
                continue;
            else
            {
                _errno = errno;
                break;
            }
        }
        else if(n == 0)
            break;
    }

    // Increment stat for bytes written
    if(n > 0)
        UFStatSystem::increment(UFStats::bytesRead, n);

    return n;
}

ssize_t UFIO::write(const void *buf, size_t nbyte, TIME_IN_US timeout)
{
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    ssize_t n = 0;;
    unsigned int amtWritten = 0;
    while(1)
    {
        n = ::write(_fd, (char*)buf+amtWritten, nbyte-amtWritten);
        if(n > 0)
        {
            amtWritten += n;
            if(amtWritten == nbyte)
            {
                UFStatSystem::increment(UFStats::bytesWritten, n);
                return amtWritten;
            }
            else
                continue;
        }
        else if(n < 0)
        {
            _errno = errno;
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                _markedActive = false;
                _errno = 0;
                while(!_markedActive)
                {
                    if(!tmpUfios->setupForWrite(this, timeout))
                        return -1;
                }
            }
            else if(errno == EINTR)
                continue;
            else
            {
                _errno = errno;
                break;
            }
        }
        else if(n == 0)
            break;
    }

    // Increment stat for bytes written
    if(n > 0)
        UFStatSystem::increment(UFStats::bytesWritten, n);

    return n;
}

bool UFIO::connect(const struct sockaddr *addr, 
               int addrlen, 
               TIME_IN_US timeout)
{
    if(!isSetup()) //create the socket and make the socket non-blocking
    {
        _errno = EINVAL;
        return false;
    }


    //find the scheduler for this request
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    while(::connect(_fd, addr, addrlen) < 0)
    {
        if(errno == EINTR)
            continue;
        else if(errno == EINPROGRESS || errno == EAGAIN)
        {
            if(!tmpUfios->setupForConnect(this, timeout))
            {
                _errno = errno;
                return false;
            }
        }
        else
        {
            _errno = errno;
            return false;
        }
    }

    return true;
}

int UFIO::sendto(const char *buf, int len, const struct sockaddr *to, int tolen, TIME_IN_US timeout)
{
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    ssize_t n = 0;;
    unsigned int amtWritten = 0;
    while(1)
    {
        n = ::sendto(_fd, buf+amtWritten, len-amtWritten, 0, to, tolen);
        if(n > 0)
        {
            amtWritten += n;
            if((int)amtWritten == len)
                return amtWritten;
            else
                continue;
        }
        else if(n < 0)
        {
            _errno = errno;
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                _markedActive = false;
                _errno = 0;
                while(!_markedActive)
                {
                    if(!tmpUfios->setupForWrite(this, timeout))
                        return -1;
                }
            }
            else if(errno == EINTR)
                continue;
            else
            {
                _errno = errno;
                break;
            }
        }
        else if(n == 0)
            break;
    }
    return n;
}

int UFIO::sendmsg(const struct msghdr *msg, 
                  int flags,
	              TIME_IN_US timeout)
{
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    ssize_t n = 0;;
    while(1)
    {
        n = ::sendmsg(_fd, msg, flags); 
        if(n > 0)
            continue;
        else if(n < 0)
        {
            _errno = errno;
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                _markedActive = false;
                _errno = 0;
                while(!_markedActive)
                {
                    if(!tmpUfios->setupForWrite(this, timeout))
                        return -1;
                }
            }
            else if(errno == EINTR)
                continue;
            else
            {
                _errno = errno;
                break;
            }
        }
        else if(n == 0)
            break;
    }
    return n;
}

int UFIO::recvfrom(char *buf, 
                   int len, 
                   struct sockaddr *from,
		           int *fromlen, 
                   TIME_IN_US timeout)
{
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    ssize_t n = 0;;
    while(1)
    {
        n = ::recvfrom(_fd, buf, len, 0, from, (socklen_t *)fromlen);
        if(n > 0)
            return n;
        else if(n < 0)
        {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                _errno = 0;
                _markedActive = false;
                while(!_markedActive)
                {
                    if(!tmpUfios->setupForRead(this, timeout))
                        return -1;
                }
                continue;
            }
            else if(errno == EINTR)
                continue;
            else
            {
                _errno = errno;
                break;
            }
        }
        else if(n == 0)
            break;
    }
    return n;
}

int UFIO::recvmsg(struct msghdr *msg, 
                  int flags,
	              TIME_IN_US timeout)
{
    UFIOScheduler* tmpUfios = _ufios ? _ufios : UFIOScheduler::getUFIOS();

    ssize_t n = 0;
    while(1)
    {
        n = ::recvmsg(_fd, msg, flags);
        if(n > 0)
            return n;
        else if(n < 0)
        {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                _errno = 0;
                _markedActive = false;
                while(!_markedActive)
                {
                    if(!tmpUfios->setupForRead(this, timeout))
                        return -1;
                }
                continue;
            }
            else if(errno == EINTR)
                continue;
            else
            {
                _errno = errno;
                break;
            }
        }
        else if(n == 0)
            break;
    }
    return n;
}


static pthread_key_t getThreadKey()
{
    if(pthread_key_create(&UFIOScheduler::_keyToIdentifySchedulerOnThread, 0) != 0)
    {
        cerr<<"couldnt create ufios thread specific key "<<strerror(errno)<<endl;
        exit(1);
    }
    return UFIOScheduler::_keyToIdentifySchedulerOnThread;
}
pthread_key_t UFIOScheduler::_keyToIdentifySchedulerOnThread = getThreadKey();

ThreadFiberIOSchedulerMap UFIOScheduler::_tfiosscheduler;
EpollUFIOScheduler::EpollUFIOScheduler(UF* uf, unsigned int maxFds)
{
    _uf = uf;
    _maxFds = maxFds;
    _epollFd = -1;
    _epollEventStruct = 0;
    _alreadySetup = false;
    _earliestWakeUpFromSleep = 0;
}

UFIOScheduler::UFIOScheduler()
{ 
    _connPool = new UFConnectionPool;
}
UFIOScheduler::~UFIOScheduler()
{ 
    delete _connPool; 
}

EpollUFIOScheduler::~EpollUFIOScheduler()
{
    if(_epollFd != -1)
        close(_epollFd);
    if(_epollEventStruct)
        free (_epollEventStruct);
}

bool EpollUFIOScheduler::isSetup()
{
    if(_alreadySetup)
        return true;

    pthread_t tid = pthread_self();
    ThreadFiberIOSchedulerMap::iterator index = _tfiosscheduler.find(tid);
    if(index != _tfiosscheduler.end())
    {
        cerr<<"UFIOScheduler* "<<index->second<<" is already associated w/ thread "<<tid<<" - cannot create two schedulers w/in one thread"<<endl;
        exit(1);
        return false;
    }
    _tfiosscheduler[tid] = this;

    if(_epollFd != -1 && _epollEventStruct)
        return true;

    if((_epollFd = epoll_create(_maxFds)) < 0)
    {
        cerr<<"couldnt create epoll object "<<strerror(errno)<<" got "<<_epollFd<<" instead"<<endl;
        return false;
    }

    _epollEventStruct = (struct epoll_event*)malloc((sizeof (struct epoll_event))*_maxFds);
    if(!_epollEventStruct)
    {
        close(_epollFd);
        _epollFd = -1;
    }

    pthread_setspecific(_keyToIdentifySchedulerOnThread, this);
    return (_alreadySetup = true);
}

bool EpollUFIOScheduler::addToScheduler(UFIO* ufio, void* inputInfo, TIME_IN_US to, bool wait, bool runEpollCtl)
{
    if(!ufio || !inputInfo || !isSetup())
    {
        ufio->_errno = EINVAL;
        return false;
    }

    if(runEpollCtl || ufio->_lastEpollFlag != *((int*)inputInfo)) //dont do anything if the flags are same as last time
    {
        struct epoll_event ev;
        ev.data.fd = ufio->getFd();
        ev.events = *((int*)inputInfo);

        int epollCtlOp = EPOLL_CTL_MOD;
        if(!ufio->getUFIOScheduler()) //the first time we're running
        {
            //keep a record of the mapping of fd to UFIO*
            ufio->setUFIOScheduler(this);
            epollCtlOp = EPOLL_CTL_ADD;
            _intUFIOMap[ufio->getFd()] = ufio;
        }

        if (epoll_ctl(_epollFd, epollCtlOp, ufio->getFd(), &ev) == -1) 
        {
            cerr<<"couldnt add/modify fd to epoll queue "<<strerror(errno)<<" trying to add "<<ufio->getFd()<<" to "<<_epollFd<<endl;
            exit(1);
            ufio->_errno = EINVAL;
            return false;
        }
        ufio->_lastEpollFlag = ev.events;
    }


    if(to) //add to the sleep queue for the epoll queue TODO
    {
        struct timeval now;
        gettimeofday(&now, 0);
        UFSleepInfo* ufsi = getSleepInfo();
        if(!ufsi)
        {
            cerr<<"couldnt create sleep info"<<endl;
            exit(1);
            ufio->_errno = EINVAL;
            return false;
        }
        ufsi->_ufio = ufio;
        ufio->_sleepInfo = ufsi;
        unsigned long long int timeToWakeUp = now.tv_sec*1000000+now.tv_usec + to;
        if(_earliestWakeUpFromSleep > timeToWakeUp ||
           !_earliestWakeUpFromSleep)
            _earliestWakeUpFromSleep = timeToWakeUp;
        _sleepList.insert(std::make_pair(timeToWakeUp, ufsi));
    }

    ufio->_errno = 0;
    ufio->setUF(_ufs->getRunningFiberOnThisThread());
    ufio->_markedActive = false;
    if(!wait)
        return true;
    ufio->getUF()->block(); //switch context till someone wakes me up

    if(!to) //nothing to do w/ no timeout
        return true;

    if(ufio->_sleepInfo)
    {
        ufio->_sleepInfo->_ufio = 0;
        ufio->_sleepInfo = 0;
        return true;
    }
    //ufio->_errno = ETIMEDOUT
    return false;
}

bool EpollUFIOScheduler::setupForConnect(UFIO* ufio, TIME_IN_US to)
{
    int flags = EPOLLOUT|EPOLLET;
    return addToScheduler(ufio, &flags, to);
}

bool EpollUFIOScheduler::setupForAccept(UFIO* ufio, TIME_IN_US to)
{
    int flags = EPOLLIN|EPOLLET;
    return addToScheduler(ufio, &flags, to);
}

bool EpollUFIOScheduler::setupForRead(UFIO* ufio, TIME_IN_US to)
{
    int flags = EPOLLIN|EPOLLET|EPOLLPRI|EPOLLERR|EPOLLHUP;
    return addToScheduler(ufio, &flags, to);
}

bool EpollUFIOScheduler::setupForWrite(UFIO* ufio, TIME_IN_US to)
{
    int flags = EPOLLOUT|EPOLLET|EPOLLPRI|EPOLLERR|EPOLLHUP;
    return addToScheduler(ufio, &flags, to);
}

bool EpollUFIOScheduler::closeConnection(UFIO* ufio)
{
    if(!ufio)
        return false;

    //remove from _intUFIOMap
    IntUFIOMap::iterator index = _intUFIOMap.find(ufio->getFd());
    if(index != _intUFIOMap.end())
        _intUFIOMap.erase(index);

    return true;
    /*
    struct epoll_event ev;
    ev.data.fd = ufio->getFd();
    ev.events = 0;
    return (epoll_ctl(_epollFd, EPOLL_CTL_DEL, ufio->getFd(), &ev) == 0) ? true : false;
    */
}

bool EpollUFIOScheduler::rpoll(list<UFIO*>& ufioList, TIME_IN_US to)
{
    int flags = EPOLLIN|EPOLLET|EPOLLPRI|EPOLLERR|EPOLLHUP;
    for(list<UFIO*>::iterator beg = ufioList.begin();
        beg != ufioList.end();
        ++beg) 
        addToScheduler(*beg, (void*)&flags, 0, false, true);

    _ufs->getRunningFiberOnThisThread()->block();
    /*
    if(!to)
        _ufs->getRunningFiberOnThisThread()->block();
    else
        _ufs->getRunningFiberOnThisThread()->usleep(to);
        */

    return true;
}


#ifndef PIPE_NOT_EFD
#include <sys/eventfd.h>
#endif
struct EpollNotifyStruct
{
    EpollNotifyStruct()
    {
        _ufios = 0;
        _efd = -1;
    }

    EpollUFIOScheduler*  _ufios;
    int                  _efd;
};

#ifdef PIPE_NOT_EFD
const char eventFDChar = 'e';
#else
const eventfd_t efdIncrementor = 1;
#endif
struct ReadNotificationUF : public UF
{
    void run()
    {
        if(!_startingArgs)
            return;

        EpollNotifyStruct* ens = (EpollNotifyStruct*)_startingArgs;
        EpollUFIOScheduler* ufios = ens->_ufios;
        UFIO* ufio = new UFIO(UFScheduler::getUF());
        ufio->setFd(ens->_efd, false);

#ifdef PIPE_NOT_EFD
        char readResult[128];
#else
        eventfd_t readEventFd;
#endif
        while(1)
        {
            ufios->setupForRead(ufio);
#ifdef PIPE_NOT_EFD
            while(1)
            {
                if(read(ufio->getFd(), &readResult, 127) == 127)
                    continue;
                break;
            }
#else
            eventfd_read(ufio->getFd(), &readEventFd); //TODO: deal w/ error case later
#endif
            ufios->_interruptedByEventFd = true;
        }

        delete ens;
        delete ufio;
    }

    ReadNotificationUF(bool registerMe = false)
    {
        if(registerMe)
            _myLoc = UFFactory::getInstance()->registerFunc((UF*)this);
    }
    UF* createUF() { return new ReadNotificationUF(); }
    static ReadNotificationUF* _self;
    static int _myLoc;
};
int ReadNotificationUF::_myLoc = -1;
ReadNotificationUF* ReadNotificationUF::_self = new ReadNotificationUF(true);

static void* notifyEpollFunc(void* args)
{
    if(!args)
        return 0;
#ifdef PIPE_NOT_EFD
    write(*((int*)args), &eventFDChar, 1);
#else
    eventfd_write(*((int*)args), efdIncrementor); //TODO: deal w/ error case later
#endif

    return 0;
}

void EpollUFIOScheduler::waitForEvents(TIME_IN_US timeToWait)
{
    _ufs = UFScheduler::getUFScheduler();
    if(!_ufs)
    {
        cerr<<"have to be able to find my scheduler"<<endl;
        return;
    }
    
    //add the notification function
    EpollNotifyStruct* ens = new EpollNotifyStruct();
    ens->_ufios = this;
#ifdef PIPE_NOT_EFD
    int pfd[2];
    if (pipe(pfd) == -1) 
    { 
        cerr<<"error in pipe creation = "<<strerror(errno)<<endl;
        exit(1);
    }
    makeSocketNonBlocking(pfd[0]);
    //makeSocketNonBlocking(pfd[1]); - dont make the write socket non-blocking
    _ufs->_notifyArgs = (void*)(&pfd[1]);
    ens->_efd = pfd[0];
#else
    int efd = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC); //TODO: check the error code of the eventfd creation
    _ufs->_notifyArgs = (void*)&efd;
    ens->_efd = efd;
#endif
    _ufs->_notifyFunc = notifyEpollFunc;
    //add the UF to handle the efds calls
    UF* eventFdFiber = new ReadNotificationUF();
    eventFdFiber->_startingArgs = ens;
    _ufs->addFiberToScheduler(eventFdFiber, 0);




    if(!_uf)
    {
        cerr<<"have to associate an user fiber with the scheduler"<<endl;
        return;
    }
    if(!isSetup())
    {
        cerr<<"have to be able to setup EpollUFIOScheduler "<<strerror(errno)<<endl;
        return;
    }

    int nfds;
    struct timeval now;
    unsigned long long int timeNow = 0;
    IntUFIOMap::iterator index;
    UFIO* ufio = 0;
    UF* uf = 0;
    unsigned long long int amtToSleep = timeToWait;
    int i = 0;
    _interruptedByEventFd = false;
    UFScheduler* ufs = _uf->getParentScheduler();
    list<UF*> ufsToAddToScheduler;
    if(!ufs)
    {
        cerr<<"epoll scheduler has to be connected to some scheduler"<<endl;
        return;
    }
    unsigned long long int amtToSleepFromUFS = 0;
    while(1)
    {
        if(_interruptedByEventFd) //this is so that the last interruption gets handled right away
        {
            _interruptedByEventFd = false;
            _uf->yield();
        }

        if(ufs->getActiveRunningListSize() < 2) //means that epoll is the only fiber thats currently active
        {
            if(amtToSleep > (amtToSleepFromUFS = ufs->getAmtToSleep()))
                amtToSleep = (int)(amtToSleepFromUFS/1000);
            else
                amtToSleep = (amtToSleep > 1000 ? (int)(amtToSleep/1000) : 1); //let epoll sleep for atleast 1ms
        }
        else
            amtToSleep = 0; //dont wait on epoll - since there are other ufs waiting to run
        nfds = ::epoll_wait(_epollFd, _epollEventStruct, _maxFds, amtToSleep);
        if(nfds > 0)
        {
            //for each of the fds that had activity activate them
            for (i = 0; i < nfds; ++i) 
            {
                index = _intUFIOMap.find(_epollEventStruct[i].data.fd);
                if(index != _intUFIOMap.end())
                {
                    ufio = index->second;
                    if(!ufio || !(uf = ufio->getUF()))
                    {
                        cerr<<"invalid user fiber io found for fd, "<<_epollEventStruct[i].data.fd<<endl;
                        exit(1);
                    }
                    ufio->_markedActive = true;
                    if(ufio->_active)
                        //activate the fiber
                        ufs->addFiberToScheduler(uf, 0);
                    //else must be the case that no one is watching this ufio - such as in the case where the conn. pool holding onto the conn.
                }
                else
                {
                    cerr<<"couldnt find the associated UF* for fd, "<<_epollEventStruct[i].data.fd<<endl;
                    exit(1);
                }
            }
        }
        else if(nfds < 0)
        {
            if(errno == EINTR)
                continue;
            cerr<<"error w/ epoll wait "<<strerror(errno)<<endl;
            exit(1);
        }

        amtToSleep = timeToWait;
        //pick up the fibers that may have completed sleeping
        //look into the sleep list;
        if(!_sleepList.empty())
        {
            gettimeofday(&now, 0);
            timeNow = (now.tv_sec*1000000)+now.tv_usec;
            if(timeNow >= _earliestWakeUpFromSleep) //dont go into this queue unless the time seen the last time has passed
            {
                ufsToAddToScheduler.clear();
                for( MapTimeUFIO::iterator beg = _sleepList.begin(); beg != _sleepList.end(); )
                {
                    //1. see if anyone has crossed the sleep timer - add them to the active list
                    if(beg->first <= timeNow) //sleep time is over
                    {
                        UFSleepInfo* ufsi = beg->second;
                        if(ufsi)
                        {
                            UFIO* ufio = ufsi->_ufio;
                            if(ufio &&
                                ufio->_sleepInfo == ufsi &&  //make sure that the ufio is not listening on another sleep counter right now
                                ufio->_uf->_status == BLOCKED) //make sure that the uf hasnt been unblocked already
                            {
                                ufio->_sleepInfo = 0;
                                ufio->_errno = ETIMEDOUT;
                                ufsToAddToScheduler.push_back(ufio->_uf);
                                //this is so that we dont have to wait to handle the conn. being woken up
                                _interruptedByEventFd = true;
                            }

                            releaseSleepInfo(*ufsi);
                        }

                        _sleepList.erase(beg);
                        beg = _sleepList.begin();
                        continue;
                    }
                    else
                    {
                        amtToSleep = (amtToSleep > beg->first-timeNow) ? beg->first-timeNow : amtToSleep;
                        _earliestWakeUpFromSleep = beg->first;
                        break;
                    }
                    ++beg;
                }
                ufs->addFiberToScheduler(ufsToAddToScheduler, 0);
            }
        }

        //take a break - let the active conns. get a chance to run
        _uf->yield();
    }

    _ufs->_notifyArgs = 0;
    _ufs->_notifyFunc = 0;
}

int IORunner::_myLoc = -1;
IORunner* IORunner::_self = new IORunner(true);
void IORunner::run()
{
    UF* uf = UFScheduler::getUF();
    //add the scheduler for this 
    EpollUFIOScheduler* ioRunner = new EpollUFIOScheduler(uf, 10000); //TODO: support other event scheduler mechanisms later
    if(!ioRunner || !ioRunner->isSetup())
    {
        cerr<<"couldnt setup epoll io scheduler object"<<endl;
        return;
    }
    ioRunner->waitForEvents(1000000); //TODO: allow to change the epoll interval later
}

void UFIO::ufCreateThreadWithIO(pthread_t* tid, list<UF*>* ufsToStartWith)
{
    ufsToStartWith->push_front(new IORunner());

    // Add connection pool cleaner
    //ufsToStartWith->push_back(new UFConnectionPoolCleaner); //TODO: figure out how to deal w/ inactive connections
    UFScheduler::ufCreateThread(tid, ufsToStartWith);
}
