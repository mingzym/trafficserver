#ifndef USERTHREADS_H
#define USERTHREADS_H

#include <sys/time.h>

#include <iostream>
#include <map>
#include <stdint.h>
#include <ext/hash_map>
#include <set>
#include <vector>
#include <list>
#include <ucontext.h>
#include <pthread.h>

using namespace std;
namespace std { using namespace __gnu_cxx; }

enum UFStatus
{
    NOT_STARTED         = 0,
    WAITING_TO_RUN      = 1,
    BLOCKED             = 2,
    RUNNING             = 3,
    COMPLETED           = 4
};

//create the type of UF you'd like to pass into the accept handler
typedef unsigned long long int UFId;
struct UFScheduler;
struct UF
{
    friend class UFScheduler;
    friend class UFMutex;

    ///the constructor
    UF();
    bool setup(void* stackPtr = 0, size_t stackSize = 0);
    virtual ~UF();

    virtual void run() = 0; 
    virtual UF* createUF() = 0; //instructs the derived classes on how to create this object


    UFScheduler*         getParentScheduler() const;
    UFId                 self() const { return _myId; }
    //must be called after the fiber is added to a scheduler
    //otherwise behavior is unexpected
    void                 yield();
    ///must be called after the fiber is added to a scheduler
    void                 usleep(unsigned long long int sleepAmtInUs);
    static void          gusleep(unsigned long long int sleepAmtInUs);
    ///simply block the fiber
    void                 block();


    UFStatus             _status;
    void*                _startingArgs;

protected:
    static UFId          _globalId;
    UFId                 _myId;
    UFScheduler*         _parentScheduler;
    ucontext_t           _UFContext;
    bool                 _UFObjectCreatedStack;

private:
    void waitOnLock();
};

struct UFFactory
{
    static UFFactory* getInstance();
    UFFactory();

    UF* selectUF(unsigned int location);;
    int registerFunc(UF* uf);

protected:
    static UFFactory*   _instance;
    UF**                _objMapping;
    size_t              _capacity;
    size_t              _size;
};
inline UFFactory* UFFactory::getInstance()
{
    return (_instance ? _instance : (_instance = new UFFactory()));
}
inline UF* UFFactory::selectUF(unsigned int location)
{
    return _objMapping[location];
}



typedef std::list<UF*>          UFList;
struct UFMutex
{
    UFMutex() 
    {
        _lockCurrentlyOwned = false;
        _pendingLockNotification = 0;
        _lockActive = 0;
    }

    bool lock(UF* uf);
    bool unlock(UF* uf);
    bool tryLock(UF* uf, unsigned long long int autoRetryIntervalInUS = 0);

    //THE CALLER MUST get the lock before calling this fxn
    //THE CALLER MUST release the lock after this fxn is called
    bool condWait(UF* uf);
    //THE CALLER MUST get the lock before calling this fxn
    //THE CALLER MUST unlock the lock after calling this fxn (to maintain parity w/ pthread calling paradigms
    void broadcast();
    //THE CALLER MUST get the lock before calling this fxn
    //THE CALLER MUST unlock the lock after calling this fxn (to maintain parity w/ pthread calling paradigms
    void signal();


protected:
    int             _lockActive;
    UFList          _listOfClientsWaitingOnLock;
    UFList          _listOfClientsWaitingOnCond;
    bool            _lockCurrentlyOwned;
    UF*             _pendingLockNotification;

    void releaseLocalLock();
    void getLocalLock();
};


//per thread scheduler
typedef std::multimap<unsigned long long int, UF*> MapTimeUF;
//typedef std::map<pthread_t,UFScheduler*> ThreadUFSchedulerMap;
typedef hash_map<pthread_t, UFScheduler*, hash<uintptr_t> > ThreadUFSchedulerMap;

struct UFConnectionPool;
struct UFScheduler
{
    friend class UF;
    friend class UFMutex;

    UFScheduler();
    ~UFScheduler();
    void runScheduler();


    //call this fxn the first time you're adding a UF 
    //(not after that - currently cant move an existing UF to a different thread)
    bool addFiberToScheduler(UF* uf,      /* the UF to add */
                                pthread_t tid = 0); /* the thread to add the UF to */
    //add the fxn to add multiple ufs in one shot (if they're on one tid)
    bool addFibersToScheduler(const std::list<UF*>& ufList, pthread_t tid = 0);



    static ThreadUFSchedulerMap  _threadUFSchedulerMap;
    static pthread_mutex_t       _mutexToCheckFiberScheduerMap;
            
    //returns the fiber scheduler on this thread or other threads;
    static UFScheduler*          getUFScheduler(pthread_t tid = 0); 
    //returns the UF running in the thread provided or the current thread
    static UF*                   getUF(pthread_t tid = 0); 

    //asks the system to work in threaded mode or not (default is yes)
    static bool                  _inThreadedMode;

    UF* getRunningFiberOnThisThread();
    const ucontext_t& getMainContext() const;
    void setSpecific(void* args);
    void* getSpecific() const;
    unsigned long long int getAmtToSleep() const;
    static void setExit(bool exit = true);
    void setExitJustMe(bool exit = true);

    ///the variable that says whether the scheduler should be handling the sleep or
    //if its handled w/in the UserFabrics
    void*                       (*_notifyFunc)(void*);
    void*                       _notifyArgs;

    //to allow to identify the thread running now
    static pthread_key_t        _specific_key;

    static void ufCreateThread(pthread_t* tid, std::list<UF*>* ufsToStartWith);

    static bool                 _exit;
    bool                        _exitJustMe;
protected:
    UF*                         _currentFiber;
    ucontext_t                  _mainContext;

    //no lock for active running list - cause only the running
    //thread can add to it
    UFList                      _activeRunningList;

    //nominate to add to a thread's running list
    UFList                      _nominateToAddToActiveRunningList;
    pthread_mutex_t             _mutexToNominateToActiveList;
    pthread_cond_t              _condToNominateToActiveList;

    //the sleep tree
    MapTimeUF                   _sleepList;
    //store the shortest sleep interval
    unsigned long long int      _amtToSleep;


    //store thread specific content
    void*                       _specific;
    pthread_t                   _tid;

    void notifyUF();

public:
    //stats for thread
    std::vector<long long> _stats;
    UFMutex _stats_lock;

    // connection pool
    UFConnectionPool *_conn_pool;
};

inline unsigned long long int UFScheduler::getAmtToSleep() const { return _amtToSleep; }
inline UF* UFScheduler::getRunningFiberOnThisThread(){ return _currentFiber; }
inline const ucontext_t& UFScheduler::getMainContext() const { return _mainContext; };
inline void UFScheduler::setSpecific(void* args) { _specific = args; }
inline void* UFScheduler::getSpecific() const { return _specific; }
inline void UFScheduler::setExit(bool exit) { _exit = exit; }
inline void UFScheduler::setExitJustMe(bool exit) { _exitJustMe = exit; }

inline UFScheduler* UF::getParentScheduler() const { return _parentScheduler; }

inline void UF::waitOnLock() { block(); }

inline void UF::gusleep(unsigned long long int sleepAmtInUs)
{
    UFScheduler::getUFScheduler()->getRunningFiberOnThisThread()->usleep(sleepAmtInUs);
}

inline void UF::usleep(unsigned long long int sleepAmtInUs)
{
    if(!sleepAmtInUs)
    {
        //yield(); //just give up control of the CPU
        return;
    }

    struct timeval now;
    gettimeofday(&now, 0);
    unsigned long long int timeNow = now.tv_sec*1000000+now.tv_usec;
    _parentScheduler->_sleepList.insert(std::make_pair((timeNow+sleepAmtInUs), this));
    block();
}

inline void UF::block()
{
    _status = BLOCKED;
    yield();
}

inline void UF::yield()
{
    //switch context back to the main scheduler
    swapcontext(&_UFContext, &(_parentScheduler->getMainContext()));
}

inline void UFMutex::releaseLocalLock()
{
    while(!__sync_bool_compare_and_swap(&_lockActive, 1, 0)) { sched_yield(); }
    //while(!__sync_bool_compare_and_swap(&_lockActive, 1, 0)) { }
}

inline void UFMutex::getLocalLock()
{
    while(!__sync_bool_compare_and_swap(&_lockActive, 0, 1)) { sched_yield(); }
    //while(!__sync_bool_compare_and_swap(&_lockActive, 0, 1)) { }
}

inline bool UFMutex::lock(UF* uf)
{
    if(!uf || !uf->_parentScheduler)
        return false;

    getLocalLock();
    if(_listOfClientsWaitingOnLock.empty()) //probably the most common case (no UF has the lock)
    {
        _listOfClientsWaitingOnLock.push_back(uf);
        _lockCurrentlyOwned = true;
        releaseLocalLock();
        return true;
    }

    //see if any UF is holding the lock right now - if not get the lock
    //this is the case where between the time that an UF is woken up
    //(after another UF releases the lock)
    //and it actually runs this requesting UF might be able to procure the lock
    if(!_lockCurrentlyOwned)
    {
        _listOfClientsWaitingOnLock.push_front(uf);
        _lockCurrentlyOwned = true;
        releaseLocalLock();
        return true;
    }

    //for the rest of the UFs that didnt meet the above criteria 
    //and didnt get the lock they have to wait
    _listOfClientsWaitingOnLock.push_back(uf);
    releaseLocalLock();

    while(1) //try to get the lock
    {
        //simply yield - since the uf will be woken up once it gets the lock
        uf->waitOnLock();

        //since this uf got woken up - check if it can get the lock now
        getLocalLock();

        if(_pendingLockNotification != uf) //there can only be one pending notification out at any time - since this lock was woken up - this must have gotten the pending notification
        {
            releaseLocalLock();
            continue;
        }
        _pendingLockNotification = 0; 

        //check if any other UF has gotten the lock between the time that this UF 
        //got the notification and actually acted on it
        if(!_lockCurrentlyOwned && (_listOfClientsWaitingOnLock.front() == uf))
        {
            _lockCurrentlyOwned = true;
            releaseLocalLock();
            return true;
        }

        releaseLocalLock();
    }

    return true;
}

inline bool UFMutex::unlock(UF* uf)
{
    if(!uf)
        return false;

    UFList::iterator beg;
    getLocalLock();

    beg = _listOfClientsWaitingOnLock.begin();
    if(uf == *beg) //check if this uf is the current owner of this lock
    {
        _lockCurrentlyOwned = false;
        beg = _listOfClientsWaitingOnLock.erase(beg);

        //notify the next UF in line
        if(!_listOfClientsWaitingOnLock.empty() && !_pendingLockNotification)
        {
            _pendingLockNotification = (*beg);
            releaseLocalLock();
            (*beg)->_parentScheduler->addFiberToScheduler((*beg), (*beg)->_parentScheduler->_tid);
        }
        else
            releaseLocalLock();

        return true;
    }

    releaseLocalLock();
    return false;
}

inline bool UFMutex::tryLock(UF* uf, unsigned long long int autoRetryIntervalInUS)
{
    while(1)
    {
        getLocalLock();
        if(_listOfClientsWaitingOnLock.empty())
        {
            _listOfClientsWaitingOnLock.push_back(uf);
            _lockCurrentlyOwned = true;
            releaseLocalLock();
            return true;
        }

        releaseLocalLock();

        if(!autoRetryIntervalInUS)
            break;

        usleep(autoRetryIntervalInUS);
    }

    return false;
}



inline bool UFMutex::condWait(UF* uf)
{
    if(!uf)
        return false;
    _listOfClientsWaitingOnCond.push_back(uf) ;
    unlock(uf);
    uf->waitOnLock(); //this fxn will cause the fxn to wait till a signal or broadcast has occurred
    lock(uf);

    return true;
}

inline void UFMutex::broadcast()
{
    if(_listOfClientsWaitingOnCond.empty())
        return;

    //notify all the waiters to wake up
    UFList::iterator beg = _listOfClientsWaitingOnCond.begin();
    for(; beg != _listOfClientsWaitingOnCond.end(); )
    {
        (*beg)->_parentScheduler->addFiberToScheduler(*beg, (*beg)->_parentScheduler->_tid);
        beg = _listOfClientsWaitingOnCond.erase(beg);
    }
}

inline void UFMutex::signal()
{
    if(_listOfClientsWaitingOnCond.empty())
        return;

    UFList::iterator beg = _listOfClientsWaitingOnCond.begin();
    (*beg)->_parentScheduler->addFiberToScheduler(*beg, (*beg)->_parentScheduler->_tid);
    _listOfClientsWaitingOnCond.erase(beg);
}

//TODO: later
/*
inline bool UFMutex::condTimedWait(UF* uf)
{
    if(!uf)
        return false;
}
*/

#endif
