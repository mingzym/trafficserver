#ifndef UFCONNECTIONPOOL_H
#define UFCONNECTIONPOOL_H

#include <time.h>

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


class StIpInfo;
class UFConnectionGroupInfo;
class UFIO;
class UFConnectionPoolImpl;
class UFConnectionPool
{
public:
    UFConnectionPool();

    // Call init before thread creation
    static void init();
    
    unsigned int loadConfigFile(const std::string& fileName);
    unsigned int loadConfigFile(const std::string& fileName, int maxSimultaneousConns);

    bool addGroup(UFConnectionGroupInfo* stGroupInfo);
    UFConnectionGroupInfo* removeGroup(const std::string& name);
    UFIO* getConnection(const std::string& groupName);
    UFIO* getConnection(const std::string& groupName, bool waitForConnection);
    void releaseConnection(UFIO* ufIO, bool connOk = true);
    void setTimeoutIP(int timeoutIn); ///how long to timeout an ip that we cant connect to (is not responding)
    std::string fillInfo(std::string& data, bool detailed = false) const;
    double getGroupAvailability(const std::string& name) const;

    //TODO - get stats & detailed stats info
protected:

    UFConnectionPoolImpl*           impl;

};

namespace StringUtil
{
    typedef std::vector<std::string> StringVector;
    typedef std::vector<std::string>::iterator StringVectorItr;
    unsigned int split(const std::string& input, const std::string& splitOn, StringVector& output);
    std::string trim_ws(const std::string& input);
}

struct UFIO;
typedef std::map<UFIO*, int> UFIOIntMap;
struct UFConnectionIpInfo
{
    UFConnectionIpInfo(const std::string& ip, bool persistent = true, int maxSimultaneousConns = -1, int _timeOutPerTransaction = 90);
    ~UFConnectionIpInfo() {} //TODO: remove all the conns w/in currently available and currently used
    std::string         _ip;
    struct sockaddr_in  _sin;
    int                 _maxSimultaneousConns;
    bool                _persistent; 

    int                 _timeOutPerTransaction; ///how many ms to try before considering the connect a failure
    int                 _timedOut;

    UFIOIntMap          _currentlyAvailableConnections;
    UFIOIntMap          _currentlyUsedConnections;
    unsigned int        _inProcessCount;
};


typedef std::vector<UFConnectionIpInfo*> UFConnectionIpInfoList;
struct UFConnectionGroupInfo
{
    UFConnectionGroupInfo(const std::string& name);
    ~UFConnectionGroupInfo();

    bool addIP(UFConnectionIpInfo* UFConnectionIpInfo); 
    UFConnectionIpInfo* removeIP(const std::string& ip);
    double getAvailability() const ;

    std::string         _name;

    UFConnectionIpInfoList          _ipInfoList;
};


#endif
