#ifndef UFCONNECTIONPOOLIMPL_H
#define UFCONNECTIONPOOLIMPL_H

#include <time.h>

#include "UF.H"
#include "UFConnectionPool.H"

class UFIO;


typedef std::map<std::string,UFConnectionGroupInfo*>                                    GroupIPMap;
typedef std::map<UFIO*, std::pair<UFConnectionIpInfo*, std::pair<bool, time_t> > >      UFConnectionIpInfoMap;
const time_t DEFAULT_TIMEOUT_OF_IP_ON_FAILURE = 10;
class UFConnectionPoolImpl
{
public:
    UFConnectionPoolImpl();
    ~UFConnectionPoolImpl();

    // Call before thread creation
    static void init();
    
    bool addGroup(UFConnectionGroupInfo* stGroupInfo);
    UFConnectionGroupInfo* removeGroup(const std::string& name);
    UFIO* getConnection(const std::string& groupName);
    UFIO* getConnection(const std::string& groupName, bool waitForConnection);
    void releaseConnection(UFIO* ufIO, bool connOk = true);
    UFConnectionGroupInfo* addGroupImplicit(const std::string& groupName);

    std::string fillInfo(std::string& data, bool detailed = false) const;
    double getGroupAvailability(const std::string& name) const;

    void clearBadConnections();


    static time_t                               _timeoutIP;  ///How long should we time out an IP if there is a failure

protected:
    GroupIPMap                                  _groupIpMap;
    UFConnectionIpInfoMap                       _UFConnectionIpInfoMap;

    UFIO* createConnection(UFConnectionIpInfo* stIpInfo);
};

#endif
