#include "UFConnectionPool.H"
#include "UFConnectionPoolImpl.H"

#include "UFIO.H"
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#ifdef USE_CARES
#include "UFAres.H"
#endif

const unsigned short int PERCENT_LOGGING_SAMPLING = 5;

using namespace std;

UFConnIPInfo::UFConnIPInfo(const string& ip, 
                                       unsigned int port,
                                       bool persistent, 
                                       int maxSimultaneousConns, 
                                       TIME_IN_US connectTimeout,
                                       TIME_IN_US timeToFailOutIPAfterFailureInSecs)
{
    _ip = ip;
    _port = port;
    _persistent = persistent;
    _maxSimultaneousConns = maxSimultaneousConns;
    _timeToFailOutIPAfterFailureInSecs = timeToFailOutIPAfterFailureInSecs;
    _connectTimeout = connectTimeout;
    _timedOut = 0;
    _inProcessCount = 0;
    _timeToExpireDNSInfo = 0;

    memset(&_sin, 0, sizeof(_sin));
    _sin.sin_family = AF_INET;
    _sin.sin_addr.s_addr = inet_addr(_ip.c_str());
    _sin.sin_port = htons(_port);
    _currentlyUsedCount = 0;
    _lastUsed = 0;
}

UFConnGroupInfo::UFConnGroupInfo(const std::string& name)
{
    _name = name;
    _timeToExpireAt = 0;
}


bool UFConnectionPoolImpl::addGroup(UFConnGroupInfo* groupInfo)
{
    if(!groupInfo)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"invalid/empty group passed"<<endl;
        return false;
    }

    if(!groupInfo->getName().length())
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"empty group name passed"<<endl;
        return false;
    }

    if(_groupIpMap.find(groupInfo->getName()) != _groupIpMap.end())
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"group with name "<<groupInfo->getName() <<" already exists"<<endl;
        return false;
    }

    _groupIpMap[groupInfo->getName()] = groupInfo;
    return true;
}

//TODO: figure out whether we want to delete the group object on the removeGroup and the destructor fxn calls
void UFConnectionPoolImpl::removeGroup(const std::string& name)
{
    GroupIPMap::iterator foundItr = _groupIpMap.find(name);
    if(foundItr == _groupIpMap.end())
        return;

    UFConnGroupInfo* removedObj = (*foundItr).second;
    delete removedObj;
    _groupIpMap.erase(foundItr);
    return;
}

UFConnGroupInfo* UFConnectionPoolImpl::addGroupImplicit(const std::string& groupName)
{
    UFConnGroupInfo* group = new UFConnGroupInfo(groupName);
    if(!group)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" couldnt allocate memory to create group obj"<<endl;
        return NULL;
    }
    
    UFConnIPInfo* ip = new UFConnIPInfo(groupName,
                                                    true, 
                                                    _maxSimulConnsPerHost,
                                                    0,
                                                    _timeToTimeoutIPAfterFailure);
    if(!ip)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" couldnt create the ip obj"<<endl;
        delete group;
        return NULL;
    }
    //group->addIP(ip);
    addGroup(group);
    return group;
}

bool UFConnectionPoolImpl::createIPInfo(const string& groupName, UFConnGroupInfo* groupInfo)
{
    UFConnIPInfoList& ipInfoList = groupInfo->getIpInfoList();

    size_t indexOfColon = groupName.find(':');
    string hostName = groupName;
    unsigned int port = 0;
    if(indexOfColon != string::npos)
    {
        hostName = groupName.substr(0, indexOfColon);
        port = atoi(groupName.substr(indexOfColon+1).c_str());
    }

    //have to figure out the hosts listed w/ 
    //
    int lowestTTL = 0;
    UFConnIPInfo* ipInfo = 0;
    string ipString;

#ifdef USE_CARES
    UFAres ufares;
    UFHostEnt* ufhe = ufares.GetHostByName(hostName.c_str());
    unsigned int numIpsFound = 0;
    if(!ufhe || !(numIpsFound = ufhe->get_nttl()))
        return false;

    ares_addrttl* results = ufhe->get_aresttl();
    for(unsigned int i = 0; i < numIpsFound; ++i)
    {
        ipString = inet_ntoa(results[i].ipaddr);
#else
    struct hostent* h = gethostbyname(hostName.c_str());
    if(!h)
        return false;

    for(unsigned int i = 0; 1; i++)
    {
        if(!h->h_addr_list[i])
            break;
        ipString = inet_ntoa(*((in_addr*)h->h_addr_list[i]));
#endif
        //check if the ip already exists in the system
        IPInfoStore::iterator index = _ipInfoStore.find(ipString);
        if(index == _ipInfoStore.end())
        {
            ipInfo = new UFConnIPInfo(ipString, port, true, 0, 0, 30);
            _ipInfoStore[ipString] = ipInfo;
            index = _ipInfoStore.find(ipString);
        }
        ipInfo = index->second;

#ifdef USE_CARES
        if(lowestTTL > results[i].ttl || !lowestTTL)
            lowestTTL = results[i].ttl;
#else
        lowestTTL = 300; //default to 60 secs
#endif
        ipInfoList.push_back(ipString);
    }

    time_t timeToExpireAt = lowestTTL + time(0);
    if(lowestTTL) 
        groupInfo->setTimeToExpireAt(timeToExpireAt);

    return true;
}

//TODO: return a ResultStructure which includes the UFConnIPInfo* along w/ the UFIO* so that the map doesnt have to be looked up on every return back of the structure
UFIO* UFConnectionPoolImpl::getConnection(const std::string& groupName, bool waitForConnection)
{
    if(!groupName.length())
        return 0;

    GroupIPMap::iterator foundItr = _groupIpMap.find(groupName);
    UFConnGroupInfo* groupInfo = NULL;
    if((foundItr == _groupIpMap.end()) || !((*foundItr).second))
        groupInfo = addGroupImplicit(groupName);
    else
        groupInfo = (*foundItr).second;
    if(!groupInfo)
        return 0;


    UFConnIPInfoList& ipInfoList = groupInfo->getIpInfoList();
    time_t currTime = time(0);
    if((groupInfo->getTimeToExpireAt() && (groupInfo->getTimeToExpireAt() < currTime)) || ipInfoList.empty())
    {
        if(!ipInfoList.empty()) //clear the existing set since the ttl expired
            ipInfoList.clear();

        if(!createIPInfo(groupName, groupInfo))
            return 0;
    }


    map<unsigned int,unsigned int> alreadySeenIPList; //this list will keep track of the ips that we've already seen
    unsigned int groupIpSize = ipInfoList.size();
    while(alreadySeenIPList.size() < groupIpSize) //bail out if we've seen all the ips already
    {
        //1a. first try to find a connection that already might exist - after that we'll try randomly picking an ip
        int elementNum = -1;
        for(unsigned int i = 0; i < groupIpSize; i++)
        {
            if(alreadySeenIPList.find(i) != alreadySeenIPList.end()) //already seen this IP
                continue;

            UFConnIPInfo* ipInfo = getIPInfo(ipInfoList[i]);
            if(ipInfo && !ipInfo->_currentlyAvailableConnections.empty())
            {
                elementNum = i;
                alreadySeenIPList[elementNum] = 1;
                break;
            }
        }

        //1b. randomly pick a host that is not timedout w/in the list of ips for the group
        if(elementNum == -1)
        {
            elementNum = random() % groupIpSize;
            if(alreadySeenIPList.find(elementNum) != alreadySeenIPList.end()) //already seen this IP
                continue;
            alreadySeenIPList[elementNum] = 1;
        }

        UFConnIPInfo* ipInfo = getIPInfo(ipInfoList[elementNum]);
        if(!ipInfo) //TODO: remove this empty ipInfo obj
            continue;

        UFIO* conn = ipInfo->getConnection(_ufConnIPInfoMap);
        if(conn)
            return conn;
    }
    
    return NULL;
}

UFConnIPInfo* UFConnectionPoolImpl::getIPInfo(const std::string& name)
{
    IPInfoStore::iterator index = _ipInfoStore.find(name);
    return ((index != _ipInfoStore.end()) ? index->second : 0);
}

UFIO* UFConnIPInfo::getConnection(UFConnIPInfoMap& _ufConnIPInfoMap)
{
    //2. while the host is timedout - pick another one (put into the list of already seen ips)
    time_t currTime = time(0);
    _lastUsed = currTime;
    if(getTimedOut() && ((unsigned int)(getTimedOut() + getTimeToFailOutIPAfterFailureInSecs()) > (unsigned int) currTime) )
        return 0;
    setTimedOut(0);

    UFIO* returnConn = NULL;
    while(1) 
    {
        if(!_currentlyAvailableConnections.empty())
        {
            //3. pick a connection from the currently available conns
            UFIOIntMap::iterator beg = _currentlyAvailableConnections.begin();
            for(; beg != _currentlyAvailableConnections.end(); 
                beg = _currentlyAvailableConnections.begin()  // we're resetting to the beginning to avoid
                    // the case of two threads ending up getting 
                    // the same connection
                )
            {
                returnConn = beg->first;
                _currentlyAvailableConnections.erase(beg);
                if(!returnConn)
                {
                    cerr<<time(0)<<" "<<__LINE__<<" "<<"found null conn - removing that from currentlyAvailable"<<endl;
                    continue;
                }

                if(returnConn->_markedActive) //this indicates that the conn. had some activity while sleeping - thats no good
                {
                    UFConnIPInfoMap::iterator index = _ufConnIPInfoMap.find(returnConn);
                    _ufConnIPInfoMap.erase(index);
                    delete returnConn;
                    continue;
                }
                returnConn->_active = true;
                _currentlyUsedCount++;
                return returnConn;
            }
        }
        else 
        {
            // if _maxSimultaneousConns is hit, wait for a connection to become available
            if(getMaxSimultaneousConns() && 
               (_currentlyUsedCount + getInProcessCount() >= (unsigned int) getMaxSimultaneousConns())) 
            {
                // wait for a connection to be released
                UF* this_user_fiber = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
                getMutexToCheckSomeConnection()->lock(this_user_fiber);
                getMutexToCheckSomeConnection()->condWait(this_user_fiber);
                getMutexToCheckSomeConnection()->unlock(this_user_fiber);
                continue;
            }
            else 
            {
                // Create a new connection
                incInProcessCount(1);
                returnConn = createConnection();
                incInProcessCount(-1);
                if(returnConn)
                {
                    _currentlyUsedCount++;
                    _ufConnIPInfoMap[returnConn] = this;
                }
                else
                {
                    if((random() % 100) < PERCENT_LOGGING_SAMPLING)
                        cerr<<time(0)<<" "<<__LINE__<<" "<<"couldnt create a connection to "<<getIP()<<" "<<strerror(errno)<<endl;
                }
                
                return returnConn;
            }
        }
    }

    return 0;
}

UFIO* UFConnIPInfo::createConnection()
{
    UFIO* ufio = new UFIO(UFScheduler::getUF());
    if(!ufio)
    {
        cerr<<"couldnt get UFIO object"<<endl;
        return NULL;
    }

    int rc = ufio->connect((struct sockaddr*) &_sin, sizeof(_sin), getConnectTimeout());
    if(rc)
        return ufio;

    cerr<<"couldnt connect to "<<getIP()<<" due to "<<strerror(ufio->getErrno())<<endl;
    delete ufio;
    return NULL;
}

const unsigned int DEFAULT_LAST_USED_TIME_INTERVAL_FOR_IP = 600;
//This fxn helps remove conns. that havent been used for a while
void UFConnectionPoolImpl::clearUnusedConnections(TIME_IN_US lastUsedTimeDiff, unsigned long long int coverListTime)
{
    if(!lastUsedTimeDiff || _ipInfoStore.empty())
        return;

    unsigned long long int sleepBetweenListElements = coverListTime / _ipInfoStore.size();
    if(sleepBetweenListElements < 1000) //atleast wait 1ms between elements in the list
        sleepBetweenListElements = 1000;

    UF* this_uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    time_t currTime = time(0);
    IPInfoStore::iterator beg = _ipInfoStore.begin();
    //walk all the ipinfo structures and then walk their available conns.
    for(; beg != _ipInfoStore.end(); )
    {
        this_uf->usleep(sleepBetweenListElements);

        UFConnIPInfo* ipInfo = beg->second;
        if(!ipInfo)
        {
            _ipInfoStore.erase(beg++);
            continue;
        }

        //walk the available connection list to see if any conn. hasnt been used for a while
        UFIOIntMap::iterator conBeg = ipInfo->_currentlyAvailableConnections.begin();
        for(; conBeg != ipInfo->_currentlyAvailableConnections.end(); )
        {
            UFIO* conn = conBeg->first;
            if(!conn) //TODO
            {
                ipInfo->_currentlyAvailableConnections.erase(conBeg++);
                continue;
            }

            if((int)(conBeg->second + lastUsedTimeDiff) < (int)currTime /*the conn. has expired*/ ||
               (conBeg->first->_markedActive)/*somehow this conn. had some update*/)
            {
                //remove the conn from the _UFConnIPInfoMap list
                UFConnIPInfoMap::iterator index = _ufConnIPInfoMap.find(conBeg->first);
                if(index != _ufConnIPInfoMap.end())
                    _ufConnIPInfoMap.erase(index);
                delete conBeg->first; //delete the connection
                ipInfo->_currentlyAvailableConnections.erase(conBeg++);
                continue;
            }
            ++conBeg;
        }
        if(ipInfo->_currentlyAvailableConnections.empty())
        {
            //check the last time this ipinfo was ever used - if its > than 300s remove it
            if(ipInfo->getLastUsed() + DEFAULT_LAST_USED_TIME_INTERVAL_FOR_IP < (unsigned int) currTime)
            {
                _ipInfoStore.erase(beg++);
                continue;
            }
        }
        ++beg;
    }
}

void UFConnectionPoolImpl::releaseConnection(UFIO* ufIO, bool connOk)
{
    if(!ufIO)
        return;

    //find the ipinfo associated w/ this connection
    UFConnIPInfoMap::iterator ufIOIpInfoLocItr = _ufConnIPInfoMap.find(ufIO);
    if((ufIOIpInfoLocItr == _ufConnIPInfoMap.end()) || !ufIOIpInfoLocItr->second)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"couldnt find the associated ipinfo object or the object was empty - not good"<<endl;
        if(ufIOIpInfoLocItr != _ufConnIPInfoMap.end())
            _ufConnIPInfoMap.erase(ufIOIpInfoLocItr);
        delete ufIO;
        return;
    }

    UFConnIPInfo* ipInfo = ufIOIpInfoLocItr->second;
    ipInfo->_currentlyUsedCount--;

    //add to the available list
    if(connOk && ipInfo->getPersistent())
    {
        time_t currTime = time(0);
        ipInfo->_currentlyAvailableConnections[ufIO] = currTime;
        ufIO->_markedActive = false;
        ufIO->_active = false;
    }
    else
    {
        _ufConnIPInfoMap.erase(ufIOIpInfoLocItr);
        delete ufIO;
        return;
    }

    //signal to all the waiting threads that there might be a connection available
    UF* this_user_fiber = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    ipInfo->getMutexToCheckSomeConnection()->lock(this_user_fiber);
    ipInfo->getMutexToCheckSomeConnection()->broadcast();
    ipInfo->getMutexToCheckSomeConnection()->unlock(this_user_fiber);
}

UFConnectionPoolImpl::~UFConnectionPoolImpl() 
{ 
}

const unsigned int DEFAULT_COVER_LIST_TIME_IN_US = 60*1000*1000;
void UFConnectionPoolCleaner::run()
{
    UF* this_uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    UFIOScheduler* ufios = UFIOScheduler::getUFIOS();
    UFConnectionPool* ufcp = ufios->getConnPool();

    if(!ufcp)
    {
        cerr<<"couldnt get conn pool on thread "<<pthread_self()<<endl;
        exit(1);
    }

    while(1)
    {
        this_uf->usleep(1000000);
        ufcp->clearUnusedConnections(300*1000*1000, DEFAULT_COVER_LIST_TIME_IN_US);
    }
}

const int MAX_SIMUL_CONNS_PER_HOST = 0;
const int DEFAULT_TIMEOUT_IN_SEC_ON_FAILURE = 10;
UFConnectionPoolImpl::UFConnectionPoolImpl()
{
    _maxSimulConnsPerHost = MAX_SIMUL_CONNS_PER_HOST;
    _timeToTimeoutIPAfterFailure = DEFAULT_TIMEOUT_IN_SEC_ON_FAILURE;
}

UFConnGroupInfo::~UFConnGroupInfo() 
{
}

void UFConnectionPoolImpl::init()
{
    static bool ranSrandom = false;
    if(!ranSrandom)
    {
        srand(getpid());
        ranSrandom = true;
    }
}

UFConnectionPool::UFConnectionPool() 
{ 
    _impl = new UFConnectionPoolImpl(); 
    if(_impl)
        _impl->init();
}

bool UFConnectionPool::addGroup(UFConnGroupInfo* groupInfo)
{
    return (_impl ? _impl->addGroup(groupInfo) : false);
}

UFIO* UFConnectionPool::getConnection(const std::string& groupName, bool waitForConnection)
{
    return (_impl ? _impl->getConnection(groupName, waitForConnection) : 0);
}

void UFConnectionPool::releaseConnection(UFIO* ufIO, bool connOk)
{
    if(_impl && ufIO)
        return _impl->releaseConnection(ufIO, connOk);
    else if(ufIO)
        delete ufIO;
}

void UFConnectionPool::setTimeToTimeoutIPAfterFailure(TIME_IN_US timeout) 
{ 
    if(_impl)
        return _impl->setTimeToTimeoutIPAfterFailure(timeout);
}

void UFConnectionPool::clearUnusedConnections(TIME_IN_US lastUsedTimeDiff, unsigned long long int coverListTime)
{
    if(_impl)
        return _impl->clearUnusedConnections(lastUsedTimeDiff, coverListTime);
}


TIME_IN_US UFConnectionPool::getTimeToTimeoutIPAfterFailure()
{
    return (_impl ? _impl->getTimeToTimeoutIPAfterFailure() : 0);
}

void UFConnectionPool::setMaxSimulConnsPerHost(int input)
{
    if(_impl)
        _impl->setMaxSimulConnsPerHost(input);
}

int UFConnectionPool::getMaxSimulConnsPerHost()
{
    return (_impl ? _impl->getMaxSimulConnsPerHost() : 0);
}
