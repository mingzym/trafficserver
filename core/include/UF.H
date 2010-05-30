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
#include <errno.h>

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
struct UFMutex;
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
    UFStatus             getStatus() const;


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
inline UFStatus UF::getStatus() const { return _status; }

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
inline UFFactory* UFFactory::getInstance() { return (_instance ? _instance : (_instance = new UFFactory())); }
inline UF* UFFactory::selectUF(unsigned int location) { return _objMapping[location]; }

struct UFWaitInfo;
typedef std::list<UFWaitInfo*> UFWaitList;
typedef map<UF*, UFWaitInfo*> UFWLHash;
typedef std::list<UF*>          UFList;
struct UFMutex
{
    UFMutex() 
    {
        _lockCurrentlyOwned = false;
        _lockActive = 0;
        _mustRunUF = 0;
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
    //THE CALLER MUST get the lock before calling this fxn
    //THE CALLER MUST unlock the lock after calling this fxn (to maintain parity w/ pthread calling paradigms
    int condTimedWait(UF *uf, unsigned long long int sleepAmtInUs);

    void releaseSpinLock(bool spinCPU = false);
    void getSpinLock(bool spinCPU = false);

protected:
    int                 _lockActive;
    UFList              _listOfClientsWaitingOnLock;
    //UFWaitList          _listOfClientsWaitingOnCond;
    UFWLHash            _listOfClientsWaitingOnCond;
    bool                _lockCurrentlyOwned;
    UF*                 _mustRunUF;
};
struct UFWaitInfo
{
    UFWaitInfo() { reset(); }
    void reset();
    UF*                 _uf;
    bool                _sleeping;
    bool                _waiting;
    UFMutex             _ctrl;
};
inline void UFWaitInfo::reset() { _uf = 0; _sleeping = false; _waiting = false; }


typedef std::multimap<unsigned long long int, UFWaitInfo*> MapTimeUF;
//typedef std::map<pthread_t,UFScheduler*> ThreadUFSchedulerMap;
//per thread scheduler
typedef hash_map<pthread_t, UFScheduler*, hash<uintptr_t> > ThreadUFSchedulerMap;

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
    bool addFibersToScheduler(const std::list<UF*>& ufList, 
                              pthread_t tid = 0);



    static ThreadUFSchedulerMap  _threadUFSchedulerMap;
    static pthread_mutex_t       _mutexToCheckFiberSchedulerMap;
            
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
    bool shouldExit() const;
    void setExitJustMe(bool exit = true);
    size_t getActiveRunningListSize() const;

    //stats for thread
    std::vector<long long> _stats;
    UFMutex _stats_lock;


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
    size_t                      _activeRunningListSize;

    //nominate to add to a thread's running list
    UFList                      _nominateToAddToActiveRunningList;
    pthread_mutex_t             _mutexToNominateToActiveList;
    pthread_cond_t              _condToNominateToActiveList;
    
    //the sleep tree
    MapTimeUF                   _sleepList;
    unsigned long long int      _earliestWakeUpFromSleep;
    //store the shortest sleep interval
    unsigned long long int      _amtToSleep;


    //store thread specific content
    void*                       _specific;
    pthread_t                   _tid;

    void notifyUF();
    
    list<UFWaitInfo*>  _availableWaitInfo;
    UFWaitInfo* getWaitInfo();
    void releaseWaitInfo(UFWaitInfo& ufsi);

public:
    UFMutex testingCondTimedWait;
};

inline size_t UFScheduler::getActiveRunningListSize() const { return _activeRunningListSize; }
inline bool UFScheduler::shouldExit() const { return (_exitJustMe || _exit) ? true : false; }
inline unsigned long long int UFScheduler::getAmtToSleep() const { return _amtToSleep; }
inline UF* UFScheduler::getRunningFiberOnThisThread(){ return _currentFiber; }
inline const ucontext_t& UFScheduler::getMainContext() const { return _mainContext; };
inline void UFScheduler::setSpecific(void* args) { _specific = args; }
inline void* UFScheduler::getSpecific() const { return _specific; }
inline void UFScheduler::setExit(bool exit) { _exit = exit; }
inline void UFScheduler::setExitJustMe(bool exit) { _exitJustMe = exit; }

inline UFWaitInfo* UFScheduler::getWaitInfo()
{
    if(!_availableWaitInfo.empty())
    {
        UFWaitInfo* ufwi = _availableWaitInfo.front();
        _availableWaitInfo.pop_front();
        ufwi->reset();
        return ufwi;
    }

    return new UFWaitInfo();
}

inline void UFScheduler::releaseWaitInfo(UFWaitInfo& ufwi)
{
    _availableWaitInfo.push_back(&ufwi);
}

inline UFScheduler* UF::getParentScheduler() const { return _parentScheduler; }

inline void UF::waitOnLock() { block(); }

inline void UF::gusleep(unsigned long long int sleepAmtInUs)
{
    UFScheduler::getUFScheduler()->getRunningFiberOnThisThread()->usleep(sleepAmtInUs);
}

inline unsigned long long int timeInUS(timeval& t)
{
    return ((unsigned long long int)(((unsigned long long int) t.tv_sec)*1000000)+(unsigned long long int) t.tv_usec);
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
    
    unsigned long long int timeToWakeUp = timeInUS(now) + sleepAmtInUs;
    if( _parentScheduler->_earliestWakeUpFromSleep > timeToWakeUp ||
       !_parentScheduler->_earliestWakeUpFromSleep)
        _parentScheduler->_earliestWakeUpFromSleep = timeToWakeUp;

    UFWaitInfo *ufwi = _parentScheduler->getWaitInfo();
    ufwi->_uf = this;
    ufwi->_sleeping = true;

    _parentScheduler->_sleepList.insert(std::make_pair(timeToWakeUp, ufwi));
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

inline void UFMutex::releaseSpinLock(bool spinCPU)
{
    while(!__sync_bool_compare_and_swap(&_lockActive, 1, 0)) { if(!spinCPU) sched_yield(); }
}

inline void UFMutex::getSpinLock(bool spinCPU)
{
    while(!__sync_bool_compare_and_swap(&_lockActive, 0, 1)) { if(!spinCPU) sched_yield(); }
}

#endif
