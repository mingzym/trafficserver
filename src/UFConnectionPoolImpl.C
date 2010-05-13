#include "UFConnectionPool.H"
#include "UFConnectionPoolImpl.H"

#include "UFIO.H"
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

const unsigned short int PERCENT_LOGGING_SAMPLING = 5;

using namespace std;

UFConnectionIpInfo::UFConnectionIpInfo(const string& ip, bool persistent, int maxSimultaneousConns, int timeOutPerTransaction)
{
    _ip = ip;
    _persistent = persistent;
    _maxSimultaneousConns = maxSimultaneousConns;
    if(timeOutPerTransaction > 0)
        _timeOutPerTransaction = timeOutPerTransaction*1000;
    else
        _timeOutPerTransaction = -1;
    _timedOut = 0;
    _inProcessCount = 0;

    size_t index = _ip.find_last_of(':'); 
    string ip_to_connect = (index == string::npos ) ? _ip : _ip.substr(0, index);
    string port = (index == string::npos ) ? "0" : _ip.substr(index+1);
    
    memset(&_sin, 0, sizeof(_sin));
    _sin.sin_family = AF_INET;
    _sin.sin_addr.s_addr = inet_addr(ip_to_connect.c_str());
    _sin.sin_port = htons(atoi(port.c_str()));
}

static void read_address(const char *str, struct sockaddr_in *sin)
{
    char host[128], *p;
    struct hostent *hp;
    short port;

    strcpy(host, str);
    if ((p = strchr(host, ':')) == NULL)
    {
        cerr<<"invalid host: "<<host<<endl;
        exit(1);
    }
    *p++ = '\0';
    port = (short) atoi(p);
    if (port < 1)
    {

        cerr<<"invalid port: "<<port<<endl;
        exit(1);
    }

    memset(sin, 0, sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);
    if (host[0] == '\0')
    {
        sin->sin_addr.s_addr = INADDR_ANY;
        return;
    }
    sin->sin_addr.s_addr = inet_addr(host);
    if (sin->sin_addr.s_addr == INADDR_NONE)
    {
        /* not dotted-decimal */
        if ((hp = gethostbyname(host)) == NULL)
        {
            cerr<<"cant resolve address "<<host<<endl;
            exit(1);
        }
        memcpy(&sin->sin_addr, hp->h_addr, hp->h_length);
    }
}

UFConnectionIpInfo* UFConnectionGroupInfo::removeIP(const string& ip)
{
    UFConnectionIpInfoList::iterator beg = _ipInfoList.begin();
    for(; beg != _ipInfoList.end(); ++beg)
    {
        if((*beg)->_ip == ip)
        {
            UFConnectionIpInfo* info = *beg;
            _ipInfoList.erase(beg);
            return info;
        }
    }
    return NULL;
}

bool UFConnectionGroupInfo::addIP(UFConnectionIpInfo* stIpInfo)
{
    if(!stIpInfo)
    {
        cerr<<"empty/invalid stIpInfo obj passed in "<<endl;
        return false;
    }

    read_address(stIpInfo->_ip.c_str(), &(stIpInfo->_sin));
    if(stIpInfo->_sin.sin_addr.s_addr == INADDR_ANY)   
    {
        cerr<<"couldnt resolve address:port = "<<stIpInfo->_ip<<endl;
        return false;
    }

    _ipInfoList.push_back(stIpInfo);
    return true;
}

double UFConnectionGroupInfo::getAvailability() const
{
    int timed_out_count = 0;
    int total_count = 0;
    UFConnectionIpInfoList::const_iterator itr = _ipInfoList.begin();
    for(;itr != _ipInfoList.end(); ++itr)
    {
        if(!(*itr))
            continue;

        total_count++;
        if((*itr)->_timedOut)
        {
            if ( ((*itr)->_timedOut + UFConnectionPoolImpl::_timeoutIP) < time(0) )
                timed_out_count++;
            else
                (*itr)->_timedOut = 0;
        }
    }

    return ((total_count > 0 ) ? ((total_count-timed_out_count)*100)/total_count : 0);
}

UFConnectionGroupInfo::UFConnectionGroupInfo(const std::string& name)
{
    _name = name;
}

UFConnectionGroupInfo::~UFConnectionGroupInfo()
{
    unsigned int ipInfoListSize = _ipInfoList.size();
    for(unsigned int i = 0; i < ipInfoListSize; ++i)
        delete _ipInfoList[i];
}

time_t UFConnectionPoolImpl::_timeoutIP = DEFAULT_TIMEOUT_OF_IP_ON_FAILURE;
bool UFConnectionPoolImpl::addGroup(UFConnectionGroupInfo* groupInfo)
{
    if(!groupInfo)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"invalid/empty group passed"<<endl;
        return false;
    }

    if(!groupInfo->_name.length())
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"empty group name passed"<<endl;
        return false;
    }

    if(_groupIpMap.find(groupInfo->_name) != _groupIpMap.end())
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"group with name "<<groupInfo->_name <<" already exists"<<endl;
        return false;
    }

    _groupIpMap[groupInfo->_name] = groupInfo;
    return true;
}

//TODO: figure out whether we want to delete the group object on the removeGroup and the destructor fxn calls
UFConnectionGroupInfo* UFConnectionPoolImpl::removeGroup(const std::string& name)
{
    GroupIPMap::iterator foundItr = _groupIpMap.find(name);
    if(foundItr == _groupIpMap.end())
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"didnt find group with name "<<name <<" to remove"<<endl;
        return NULL;
    }

    UFConnectionGroupInfo* removedObj = (*foundItr).second;
    _groupIpMap.erase(foundItr);
    return removedObj;
}

UFIO* UFConnectionPoolImpl::getConnection(const std::string& groupName)
{
    return getConnection(groupName, false);
}

static int MAX_SIMUL_CONNS_PER_HOST = 5;
static int TIMEOUT_PER_REQUEST = 10;
UFConnectionGroupInfo* UFConnectionPoolImpl::addGroupImplicit(const std::string& groupName)
{
    UFConnectionGroupInfo* group = new UFConnectionGroupInfo(groupName);
    if(!group)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" couldnt allocate memory to create group obj"<<endl;
        return NULL;
    }
    
    UFConnectionIpInfo* ip = new UFConnectionIpInfo(groupName,
                                                    true, 
                                                    MAX_SIMUL_CONNS_PER_HOST,
                                                    TIMEOUT_PER_REQUEST);
    if(!ip)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" couldnt create the ip obj"<<endl;
        delete group;
        return NULL;
    }
    group->addIP(ip);
    addGroup(group);
    return group;
}

UFIO* UFConnectionPoolImpl::getConnection(const std::string& groupName, bool waitForConnection)
{
    if(!groupName.length())
        return NULL;

    GroupIPMap::iterator foundItr = _groupIpMap.find(groupName);
    UFConnectionGroupInfo* groupInfo = NULL;
    if((foundItr == _groupIpMap.end()) || !((*foundItr).second))
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"null group or didnt find group with name "<<groupName<<endl;
        groupInfo = addGroupImplicit(groupName);
        if(!groupInfo)
            return NULL;
    }
    else
    {
        groupInfo = (*foundItr).second;
    }


    UFIO* returnConn = NULL;
    map<unsigned int,unsigned int> alreadySeenIPList; //this list will keep track of the ips that we've already seen
    unsigned int groupIpSize = groupInfo->_ipInfoList.size();
    while(alreadySeenIPList.size() < groupIpSize) //bail out if we've seen all the ips already
    {
        //1a. first try to find a connection that already might exist - after that we'll try randomly picking an ip
        int elementNum = -1;
        for(unsigned int i = 0; i < groupIpSize; i++)
        {
            if(alreadySeenIPList.find(i) != alreadySeenIPList.end()) //already seen this IP
                continue;

            UFConnectionIpInfo* ipInfo = groupInfo->_ipInfoList[i]; 
            if(ipInfo && ipInfo->_currentlyAvailableConnections.size())
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

        UFConnectionIpInfo* ipInfo = groupInfo->_ipInfoList[elementNum]; 
        if(!ipInfo)
            //TODO: remove this empty ipInfo obj
            continue;

        //2. while the host is timedout - pick another one (put into the list of already seen ips)
        if(ipInfo->_timedOut && ((ipInfo->_timedOut + _timeoutIP) > time(NULL)) )
            continue;
        ipInfo->_timedOut = 0;

        //3. pick a connection from the currently available conns
        //   (if there are any available)
        UFIOIntMap::iterator beg = ipInfo->_currentlyAvailableConnections.begin();
        for(; beg != ipInfo->_currentlyAvailableConnections.end(); 
              beg = ipInfo->_currentlyAvailableConnections.begin()  // we're resetting to the beginning to avoid
                                                                    // the case of two threads ending up getting 
                                                                    // the same connection
           )
        {
            returnConn = NULL;
            if(!((*beg).first))
            {
                ipInfo->_currentlyAvailableConnections.erase(beg);
                cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"found null conn - removing that from currentlyAvailable"<<endl;
                continue;
            }
            returnConn = (*beg).first;
            //take the found connection away from the curentlyAvaliableConnections list
            //since validConnection now actually checks to see the content thats within the channel to verify the validity of the connection
            //it may be that the thread gets switched out and some other thread comes into this section
            ipInfo->_currentlyAvailableConnections.erase(beg);
            ipInfo->_currentlyUsedConnections[returnConn] = time(NULL);
            break;
        }

        //4. if no connections are available then create a new connection if allowed
        if(!returnConn)
        {
            bool getConnection = false;

            if ((ipInfo->_maxSimultaneousConns < 0) || 
                (ipInfo->_currentlyAvailableConnections.size() + ipInfo->_currentlyUsedConnections.size() + ipInfo->_inProcessCount < (unsigned int) ipInfo->_maxSimultaneousConns)
               )
                getConnection = true;
            else if ( waitForConnection && 
                      (ipInfo->_currentlyAvailableConnections.size() + ipInfo->_currentlyUsedConnections.size() + ipInfo->_inProcessCount >= (unsigned int) ipInfo->_maxSimultaneousConns)
                    )
            {
                //wait for the signal to get pinged
                //unsigned short int counter = 0;
                //while(counter < MAX_WAIT_FOR_CONNECTION_TO_BE_AVAILABLE) //we only try for 10 times
                while(true)
                {
                    if (ipInfo->_currentlyAvailableConnections.size() + ipInfo->_currentlyUsedConnections.size() + ipInfo->_inProcessCount
                            >= (unsigned int) ipInfo->_maxSimultaneousConns)
                    {
                        UFScheduler* this_thread_scheduler = UFScheduler::getUFScheduler(pthread_self());
                        UF* this_user_fiber = this_thread_scheduler->getRunningFiberOnThisThread();
                        _someConnectionAvailable.lock(this_user_fiber);
                        _someConnectionAvailable.condWait(this_user_fiber);
                        _someConnectionAvailable.unlock(this_user_fiber);
                    }
                    else
                    {
                        getConnection = true; //this is here just so that if we ever go to the path of having a 
                                              //max allowed waiting time - this var. will only get set if the 
                                              //condition is met
                        break;
                    }
                }
            }

            if(getConnection)
            {
                ipInfo->_inProcessCount++;
                returnConn = createConnection(ipInfo);
                ipInfo->_inProcessCount--;
                if(returnConn)
                {
                    time_t currTime = time(NULL);
                    ipInfo->_currentlyUsedConnections[returnConn] = currTime;
                    _UFConnectionIpInfoMap[returnConn] = make_pair(ipInfo, make_pair(true, currTime));
                }
                else
                {
                    if((random() % 100) < PERCENT_LOGGING_SAMPLING)
                        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"couldnt create a connection to "<<ipInfo->_ip<<" "<<strerror(errno)<<endl;
                }
            }
        }

        if(returnConn)
            break;
    }


    //5. return the found connection
    return returnConn;
}

UFIO* UFConnectionPoolImpl::createConnection(UFConnectionIpInfo* ipInfo)
{
    if(!ipInfo)
        return NULL;
    
    UFIO* ufio = new UFIO(UFScheduler::getUF());
    if(!ufio)
    {
        cerr<<"couldnt get UFIO object"<<endl;
        return NULL;
    }

    ufio->connect((struct sockaddr *) &ipInfo->_sin, sizeof(ipInfo->_sin), 16000);

    return ufio;
}

//This fxn helps remove conns. that may have been invalidated while being in the waiting to be used state
const unsigned int LAST_USED_TIME_DIFF = 300;
void UFConnectionPoolImpl::clearBadConnections()
{
    time_t currTime = time(NULL);
    UFConnectionIpInfoMap::iterator beg = _UFConnectionIpInfoMap.begin();
    for(; beg != _UFConnectionIpInfoMap.end(); )
    {
        UFConnectionIpInfo* ip = beg->second.first;
        bool currentlyUsed = beg->second.second.first;
        if(currentlyUsed) //we dont remove conns that are in use
        {
            ++beg;
            continue;
        }

        time_t lastUsed = beg->second.second.second;
        UFIO* ufio = beg->first;
        if(!ip || 
           !ufio || 
           ((lastUsed + LAST_USED_TIME_DIFF) < (unsigned int) currTime))
        {
            ++beg;
            releaseConnection(ufio, false);
            continue;
        }
        ++beg;
    }
}

void UFConnectionPoolImpl::releaseConnection(UFIO* ufIO, bool connOk)
{
    if(!ufIO)
        return;

    //find the ipinfo associated w/ this connection
    UFConnectionIpInfoMap::iterator ufIOIpInfoLocItr = _UFConnectionIpInfoMap.find(ufIO);
    if((ufIOIpInfoLocItr == _UFConnectionIpInfoMap.end()) || !(*ufIOIpInfoLocItr).second.first)
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"couldnt find the associated ipinfo object or the object was empty - not good"<<endl;
        if(ufIOIpInfoLocItr != _UFConnectionIpInfoMap.end())
            _UFConnectionIpInfoMap.erase(ufIOIpInfoLocItr);
        ufIO = NULL;
        return;
    }

    UFConnectionIpInfo* ipInfo = (*ufIOIpInfoLocItr).second.first;
    //remove the conn from the ipInfo->_currentlyUsedConnections list 
    UFIOIntMap::iterator currUsedConnItr = ipInfo->_currentlyUsedConnections.find(ufIO);
    if(currUsedConnItr != ipInfo->_currentlyUsedConnections.end())
        ipInfo->_currentlyUsedConnections.erase(currUsedConnItr);
    else
    {
        //see if the conn is in the available connection list
        currUsedConnItr = ipInfo->_currentlyAvailableConnections.find(ufIO);
        if(currUsedConnItr != ipInfo->_currentlyAvailableConnections.end())
            ipInfo->_currentlyAvailableConnections.erase(currUsedConnItr);
        else
            cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"couldnt find the release connection in either the used or available list - not good"<<endl;

        delete ufIO;
        ufIO = NULL;
        _UFConnectionIpInfoMap.erase(ufIOIpInfoLocItr);
        return;
    }


    if(connOk && ipInfo->_persistent)
    {
        time_t currTime = time(NULL);
        (*ufIOIpInfoLocItr).second.second.first = false;
        (*ufIOIpInfoLocItr).second.second.second = currTime;
        ipInfo->_currentlyAvailableConnections[ufIO] = currTime;
    }
    else
    {
        delete ufIO;
        ufIO = NULL;
        _UFConnectionIpInfoMap.erase(ufIOIpInfoLocItr);
    }

    //signal to all the waiting threads that there might have been some change
    UFScheduler* this_thread_scheduler = UFScheduler::getUFScheduler(pthread_self());
    UF* this_user_fiber = this_thread_scheduler->getRunningFiberOnThisThread();
                        
    _someConnectionAvailable.lock(this_user_fiber);
    _someConnectionAvailable.broadcast();
    _someConnectionAvailable.unlock(this_user_fiber);
}


const unsigned int PRINT_BUFFER_LENGTH = 256*1024;
string UFConnectionPoolImpl::fillInfo(string& data, bool detailed) const
{
    char* printBuffer = new char[256*1024];
    if(!printBuffer)
        return data;

    if(!detailed)
        snprintf(printBuffer, 1024, "ConnectionInfo:\n%15s%10s%10s\n", "GroupName", "IpCount", "IP Avail.");
    else
        snprintf(printBuffer, 1024,  
            "ConnectionInfo:\n%15s%10s%10s%35s%10s%10s%10s%10s%10s\n", 
            "GroupName", 
            "IpCount", 
            "IP Avail.", 
            "IP name", 
            "isPersis", 
            "TimedOut", 
            "#Run", 
            "#InProc.", 
            "#Avail.");


    //1. list out the current groups 
    GroupIPMap::const_iterator groupIpMapItr = _groupIpMap.begin();
    for(; groupIpMapItr != _groupIpMap.end() ; ++groupIpMapItr)
    {
        UFConnectionGroupInfo* tmpGroup = (*groupIpMapItr).second;
        if(!tmpGroup)
            continue;

        unsigned int amtCopied = strlen(printBuffer);
        if((amtCopied + 1024) >= PRINT_BUFFER_LENGTH) //we can't add anymore
            break;

        snprintf(printBuffer+strlen(printBuffer), 
            1024, 
            "%15s%10d%10d\n", 
            (*groupIpMapItr).first.c_str(), 
            (int) tmpGroup->_ipInfoList.size(), 
            (int) tmpGroup->getAvailability());


        if(detailed)
        {
            UFConnectionIpInfoList::const_iterator _ipInfoListItr = tmpGroup->_ipInfoList.begin();
            for(; _ipInfoListItr != tmpGroup->_ipInfoList.end(); ++_ipInfoListItr)
            {
                UFConnectionIpInfo* ipInfo = (*_ipInfoListItr);
                if(!ipInfo)
                    continue;

                snprintf(printBuffer+strlen(printBuffer), 
                         1024, 
                         "%70s%10d%10d%10d%10d%10d\n", 
                         ipInfo->_ip.c_str(), 
                         (int) ipInfo->_persistent, 
                         (int) ipInfo->_timedOut, 
                         (int) ipInfo->_currentlyUsedConnections.size(), 
                         (int) ipInfo->_inProcessCount, 
                         (int) ipInfo->_currentlyAvailableConnections.size());
            }
        }
    }

    data.append(printBuffer);
    delete printBuffer;
    return data;
}

double UFConnectionPoolImpl::getGroupAvailability(const std::string& name) const
{
    double result = 0;
    if(!name.length())
        return result;

    GroupIPMap::const_iterator foundItr = _groupIpMap.find(name);
    if((foundItr == _groupIpMap.end()) || !((*foundItr).second))
    {
        cerr<<getpid()<<" "<<time(NULL)<<" "<<__LINE__<<" "<<"null group or didnt find group with name "<<((*foundItr).second ? (*foundItr).second->_name : "")<<endl;
        return result;
    }

    return (*foundItr).second->getAvailability();
}

UFConnectionPoolImpl::~UFConnectionPoolImpl()
{ 
    for(GroupIPMap::iterator beg = _groupIpMap.begin(); beg != _groupIpMap.end(); ++beg)
        delete beg->second;
    _groupIpMap.clear();
}

void UFConnectionPoolCleaner::run()
{
    UF* uf = UFScheduler::getUF();
    UFScheduler* ufs = uf->getParentScheduler();
    while(1)
    {
        uf->usleep(300*1000*1000);
        if(!_conn_pool)
            break;
        _conn_pool->clearBadConnections();
    }
    ufs->setExit();
}

UFConnectionPoolImpl::UFConnectionPoolImpl()
{ 
    // add fiber to monitor connections on thread
    // if (!thread_create(runThreadToMontiorBadConnections, this, 0, 4*1024))
    //    cerr<<"couldnt create thread to monitor bad connections"<<endl;
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
    impl = new UFConnectionPoolImpl(); 
}

void UFConnectionPool::init()
{
    UFConnectionPoolImpl::init();
}

UFConnectionGroupInfo* UFConnectionPool::removeGroup(const string& name)
{
    if(!impl)
        return NULL;
    return impl->removeGroup(name);
}

bool UFConnectionPool::addGroup(UFConnectionGroupInfo* stGroupInfo)
{
    if(!impl)
        return false;
    return impl->addGroup(stGroupInfo);
}

UFIO* UFConnectionPool::getConnection(const std::string& groupName, bool waitForConnection)
{
    if(!impl)
        return NULL;
    return impl->getConnection(groupName, waitForConnection);
}

UFIO* UFConnectionPool::getConnection(const string& groupName)
{
    if(!impl)
        return NULL;
    return impl->getConnection(groupName);
}

void UFConnectionPool::releaseConnection(UFIO* ufIO, bool connOk)
{
    if(!impl)
        return;
    return impl->releaseConnection(ufIO, connOk);
}

void UFConnectionPool::setTimeoutIP(int timeout) 
{ 
    if(!impl)
        return;
    if(timeout > -1)
        impl->_timeoutIP = timeout; 
    else
        impl->_timeoutIP = 60;
}

string UFConnectionPool::fillInfo(string& data, bool detailed) const
{
    if(!impl)
        return string("");
    return impl->fillInfo(data, detailed);
}

double UFConnectionPool::getGroupAvailability(const std::string& name) const
{
    if(!impl)
        return 0;
    return impl->getGroupAvailability(name);
}

void UFConnectionPool::clearBadConnections()
{
    if(!impl)
        return;
    return impl->clearBadConnections();
}

string StringUtil::trim_ws(const string& input)
{
    if(!input.length())
        return input;

    size_t beg_position = input.find_first_not_of(" \n\r\t\r");
    size_t end_position = input.find_last_not_of(" \n\t\r");

    if(beg_position == string::npos)
        beg_position = 0;
    if(end_position == string::npos)
        end_position = input.length();

    return (input.substr(beg_position, (end_position-beg_position+1)));
}

unsigned int StringUtil::split(const string& input, const string& splitOn, StringVector& output)
{
    unsigned int copyStringBegin = 0;
    output.clear();

    while(copyStringBegin < input.length())
    {
        string::size_type findLoc = input.find(splitOn, copyStringBegin);
        if(copyStringBegin != findLoc)      
        {
            string subStr = input.substr(copyStringBegin, (findLoc == string::npos) ? input.length()-copyStringBegin : findLoc - copyStringBegin);
            if(subStr.length())
                output.push_back(subStr);
            if(findLoc == string::npos)
                break;
            copyStringBegin += subStr.length();
        }
        else
            copyStringBegin += splitOn.length();
    }

    return output.size();
}

unsigned int UFConnectionPool::loadConfigFile(const string& fileName)
{
    return loadConfigFile(fileName, -1);
}

unsigned int UFConnectionPool::loadConfigFile(const string& fileName, int maxSimultaneousConns)
{
    ifstream infile(fileName.c_str());
    if(!infile)
        return false;

    int num_groups_added = 0;
    string line;
    while(getline(infile, line))
    {
        if(line.find('#') != string::npos) //bail if we see # in the line
            continue;

        //split on ':'
        StringUtil::StringVector compVec;
        int numFound = StringUtil::split(line, ":", compVec);
        if(numFound < 3)
            continue;
        string farmId = StringUtil::trim_ws(compVec[0]);
        string timeOut = StringUtil::trim_ws(compVec[1]);
        string ipList = StringUtil::trim_ws(compVec[2]);

        if(!farmId.length() || !ipList.length() || (!timeOut.length()) || (timeOut == "*"))
            continue;

        StringUtil::StringVector ipVec;
        numFound = StringUtil::split(ipList, ",", ipVec);
        if(!numFound)
        {
            cerr<<"couldnt add "<<farmId<<" because the number of ips found was 0"<<endl;
            continue;
        }

        //create the group
        UFConnectionGroupInfo* tmpGroupInfo = new UFConnectionGroupInfo(farmId);
        //create the ips and add them to the group
        for(unsigned int i = 0; i < ipVec.size(); i++)
        {
            string ip = StringUtil::trim_ws(ipVec[i]);
            ip.append(":1971");
            UFConnectionIpInfo* tmpIpInfo = new UFConnectionIpInfo(ip, ((timeOut == "-1") ? true : false), maxSimultaneousConns);

            tmpGroupInfo->addIP(tmpIpInfo);
        }

        //add the group to the connection pool
        if(!addGroup(tmpGroupInfo))
            cerr<<"couldnt add group "<<farmId<<" to the connection pool"<<endl;

        num_groups_added++;
    }

    return num_groups_added;
}

