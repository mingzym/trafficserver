#include "UF.H"
#include "UFConnectionPool.H"

#include <string.h>

#include <iostream>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>

using namespace std;

static void runFiber(void* args)
{
    if(!args)
        return;

    UF* uf = (UF*)args;
    uf->run();
    uf->_status = COMPLETED;
}

///////////////UF/////////////////////
UFFactory* UFFactory::_instance = 0;;
const unsigned int DEFAULT_STACK_SIZE = 4*4096;
UFId UF::_globalId = 0;
UF::UF()
{ 
    _startingArgs = 0;
    setup();
}

UF::~UF()
{
    if(_UFObjectCreatedStack && _UFContext.uc_stack.ss_sp)
        free(_UFContext.uc_stack.ss_sp);
}

bool UF::setup(void* stackPtr, size_t stackSize)
{
#ifdef DEBUG
    static int pageSize = sysconf(_SC_PAGE_SIZE);
    if(pageSize == -1)
    {
        cerr<<"couldnt get sysconf for pageSize "<<strerror(errno)<<endl;
        exit(1);
    }
#endif

    _myId = ++_globalId;  //TODO: make atomic
    _status = NOT_STARTED;

    if(!stackPtr || !stackSize)
    {
#ifndef DEBUG
        _UFContext.uc_stack.ss_size = (stackSize) ? stackSize : DEFAULT_STACK_SIZE;
        _UFContext.uc_stack.ss_sp = (void*) malloc (_UFContext.uc_stack.ss_size);
#else
        _UFContext.uc_stack.ss_size = DEFAULT_STACK_SIZE;
        _UFContext.uc_stack.ss_sp = (void*) memalign (pageSize, DEFAULT_STACK_SIZE);
        if(!_UFContext.uc_stack.ss_sp)
        {
            cerr<<"couldnt allocate space from memalign "<<strerror(errno)<<endl;
            exit(1);
        }
        if (mprotect((char*)_UFContext.uc_stack.ss_sp+(pageSize*3), pageSize, PROT_NONE) == -1)
        {
            cerr<<"couldnt mprotect location "<<strerror(errno)<<endl;
            exit(1);
        }
#endif
        _UFObjectCreatedStack = true;
    }
    else
    {
        _UFContext.uc_stack.ss_size = stackSize;
        _UFContext.uc_stack.ss_sp = stackPtr;
        _UFObjectCreatedStack = false;
    }
    _UFContext.uc_stack.ss_flags = 0;

    _parentScheduler = 0;

    return true;
}






///////////////UFScheduler/////////////////////
ThreadUFSchedulerMap UFScheduler::_threadUFSchedulerMap;
pthread_mutex_t UFScheduler::_mutexToCheckFiberSchedulerMap = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t getThreadKey()
{
    if(pthread_key_create(&UFScheduler::_specific_key, 0) != 0)
    {
        cerr<<"couldnt create thread specific key "<<strerror(errno)<<endl;
        exit(1);
    }
    return UFScheduler::_specific_key;
}
pthread_key_t UFScheduler::_specific_key = getThreadKey();
UFScheduler::UFScheduler()
{
    _activeRunningListSize = 0;
    _earliestWakeUpFromSleep = 0;
    _exitJustMe = false;
    _specific = 0;
    _currentFiber = 0;

    if(_inThreadedMode)
    {
        pthread_mutex_init(&_mutexToNominateToActiveList, NULL);
        pthread_cond_init(&_condToNominateToActiveList, NULL);
    }


    //check that there are no other schedulers already running in this thread
    if(_inThreadedMode)
    {
        pthread_t currThreadId = pthread_self();

        pthread_mutex_lock(&_mutexToCheckFiberSchedulerMap);
        if(_threadUFSchedulerMap.find(currThreadId) != _threadUFSchedulerMap.end())
        {
            cerr<<"cannot have more than one scheduler per thread"<<endl;
            exit(1);
        }
        _threadUFSchedulerMap[currThreadId] = this;
        pthread_mutex_unlock(&_mutexToCheckFiberSchedulerMap);
    }
    else
    {
        if(_threadUFSchedulerMap.find(0) != _threadUFSchedulerMap.end())
        {
            cerr<<"cannot have more than one scheduler per thread"<<endl;
            exit(1);
        }

        //for non-threaded mode we consider the pthread_t id to be 0
        _threadUFSchedulerMap[0] = this;
    }

    _tid = (_inThreadedMode) ? pthread_self() : 0;
    _notifyFunc = 0;
    _notifyArgs = 0;

    pthread_setspecific(_specific_key, this);
    _amtToSleep = 0;
}

UFScheduler::~UFScheduler()
{
    //pthread_key_delete(_specific_key);
}


bool UFScheduler::addFiberToScheduler(UF* uf, pthread_t tid)
{
    if(!uf)
    {
        cerr<<"returning cause there is a scheduler already"<<endl;
        return false;
    }

    list<UF*> ufList;
    ufList.push_back(uf);
    return addFibersToScheduler(ufList, tid);
}

bool UFScheduler::addFibersToScheduler(const list<UF*>& ufList, pthread_t tid)
{
    if(ufList.empty())
        return true;

    UF* uf = 0;
    list<UF*>::const_iterator beg = ufList.begin();
    list<UF*>::const_iterator ending = ufList.end();
    //adding to the same scheduler and as a result thread as the current job
    if(!tid || (tid == pthread_self()))
    {
        for(; beg != ending; ++beg)
        {
            uf = *beg;
            if(uf->_status == WAITING_TO_RUN) //UF is already in the queue
                continue;
            uf->_status = WAITING_TO_RUN;
            if(uf->_parentScheduler) //probably putting back an existing uf into the active list
            {
                if(uf->_parentScheduler == this) //check that we're scheduling for the same thread
                {
                    _activeRunningList.push_back(uf); ++_activeRunningListSize;
                    continue;
                }
                else
                {
                    cerr<<uf<<" uf is not part of scheduler, "<<this<<" its part of "<<uf->_parentScheduler<<endl;
                    abort(); //TODO: remove the abort
                    return false;
                }
            }

            //create a new context
            uf->_parentScheduler = this;
            uf->_UFContext.uc_link = &_mainContext;

            getcontext(&(uf->_UFContext));
            errno = 0;
            makecontext(&(uf->_UFContext), (void (*)(void)) runFiber, 1, (void*)uf);
            if(errno != 0)
            {
                cerr<<"error while trying to run makecontext"<<endl;
                return false;
            }
            _activeRunningList.push_back(uf); ++_activeRunningListSize;
        }
    }
    else //adding to some other threads' scheduler
    {
        //find the other thread -- 
        //TODO: have to lock before looking at this map - 
        //since it could be changed if more threads are added later - not possible in the test that is being run (since the threads are created before hand)
        ThreadUFSchedulerMap::iterator index = _threadUFSchedulerMap.find(tid);
        if(index == _threadUFSchedulerMap.end())
        {
            cerr<<"couldnt find the scheduler associated with "<<tid<<" for uf = "<<*beg<<endl;
            ThreadUFSchedulerMap::iterator beg = _threadUFSchedulerMap.begin();
            return false;
        }

        UFScheduler* ufs = index->second;
        pthread_mutex_lock(&(ufs->_mutexToNominateToActiveList));
        for(; beg != ending; ++beg)
        {
            uf = *beg;
            ufs->_nominateToAddToActiveRunningList.push_back(uf);
        }
        pthread_cond_signal(&(ufs->_condToNominateToActiveList));
        pthread_mutex_unlock(&(ufs->_mutexToNominateToActiveList));
        ufs->notifyUF();
    }

    return true;
}

void UFScheduler::notifyUF()
{
    if(_notifyFunc)
        _notifyFunc(_notifyArgs);
}


bool UFScheduler::_exit = false;
const unsigned int DEFAULT_SLEEP_IN_USEC = 1000000;
void UFScheduler::runScheduler()
{
    errno = 0;

    _amtToSleep = DEFAULT_SLEEP_IN_USEC;
    bool ranGetTimeOfDay = false;

    UFList::iterator beg;
    struct timeval now;
    struct timeval start,finish;
    gettimeofday(&start, 0);
    unsigned long long int timeNow = 0;

    UFList::iterator ufBeg;
    UFList::iterator nBeg;
    MapTimeUF::iterator slBeg;
    bool waiting = false;
    while(!shouldExit())
    {
        for(ufBeg = _activeRunningList.begin(); ufBeg != _activeRunningList.end(); )
        {
            UF* uf = *ufBeg;
            _currentFiber = uf;
            uf->_status = RUNNING;
            swapcontext(&_mainContext, &(uf->_UFContext));
            _currentFiber = 0;

            if(uf->_status == RUNNING) { }
            else if(uf->_status == BLOCKED)
            {
                ufBeg = _activeRunningList.erase(ufBeg); --_activeRunningListSize;
                continue;
            }
            else if(uf->_status == COMPLETED) 
            {
                delete uf;
                ufBeg = _activeRunningList.erase(ufBeg); --_activeRunningListSize;
                continue;
            }

            uf->_status = WAITING_TO_RUN;
            ++ufBeg;
        }


        //check the sleep queue
        ranGetTimeOfDay = false;
        _amtToSleep = DEFAULT_SLEEP_IN_USEC;

        //check if some other thread has nominated some user fiber to be
        //added to this thread's list -
        //can happen in the foll. situations
        //1. the main thread is adding a new user fiber
        //2. some fiber has requested to move to another thread
        if(!_nominateToAddToActiveRunningList.empty() /*TODO: take this out later w/ the atomic size count*/ &&
           _inThreadedMode)

        {
            _amtToSleep = 0; //since we're adding new ufs to the list we dont need to sleep
            //TODO: do atomic comparison to see if there is anything in 
            //_nominateToAddToActiveRunningList before getting the lock
            pthread_mutex_lock(&_mutexToNominateToActiveList);
            for(nBeg = _nominateToAddToActiveRunningList.begin();
                nBeg != _nominateToAddToActiveRunningList.end(); )
            {
                UF* uf = *nBeg;
                if(uf->_parentScheduler)
                {
                    uf->_status = WAITING_TO_RUN;
                    _activeRunningList.push_front(uf); ++_activeRunningListSize;
                }
                else //adding a new fiber
                    addFiberToScheduler(uf, 0);
                nBeg = _nominateToAddToActiveRunningList.erase(nBeg);
            }

            pthread_mutex_unlock(&_mutexToNominateToActiveList);
        }


        //pick up the fibers that may have completed sleeping
        //look into the sleep list;
        //printf("%u %u tnc = %llu %llu\n", (unsigned int)pthread_self(), _sleepList.size(), _earliestWakeUpFromSleep, _earliestWakeUpFromSleep-timeNow);
        if(!_sleepList.empty())
        {
            gettimeofday(&now, 0);
            ranGetTimeOfDay = true;
            timeNow = timeInUS(now);
            if(timeNow >= _earliestWakeUpFromSleep) //dont go into this queue unless the time seen the last time has passed
            {
                for(slBeg = _sleepList.begin(); slBeg != _sleepList.end(); )
                {
                    //1. see if anyone has crossed the sleep timer - add them to the active list
                    if(slBeg->first <= timeNow) //sleep time is over
                    {
                        UFWaitInfo *ufwi = slBeg->second;
                        ufwi->_ctrl.getSpinLock();
                        ufwi->_sleeping = false;
                        if(ufwi->_uf)
                        {
                            ufwi->_uf->_status = WAITING_TO_RUN;
                            _activeRunningList.push_front(ufwi->_uf); ++_activeRunningListSize;
                            ufwi->_uf = NULL;
                        }
                        waiting = ufwi->_waiting;
                        ufwi->_ctrl.releaseSpinLock();
                        if(!waiting) //since the uf is not being waited upon release it (the sleeping part has already been done)
                            releaseWaitInfo(*ufwi);
                        
                        _sleepList.erase(slBeg);
                        slBeg = _sleepList.begin();
                        
                        continue;
                    }
                    else
                    {
                        if(_amtToSleep) //since the nominate system might have turned off the sleep - we dont activate it again
                            _amtToSleep = slBeg->first-timeNow; 
                        _earliestWakeUpFromSleep = slBeg->first;
                        break;
                    }
                    ++slBeg;
                }
            }
        }

        //see if there is anything to do or is it just sleeping time now
        if(!_notifyFunc && !_activeRunningListSize && !shouldExit())
        {
            if(_inThreadedMode) //go to conditional wait (in threaded mode)
            {
                struct timespec ts;
                unsigned long long int nSecToIncrement = (int)(_amtToSleep/1000000);
                unsigned long long int nUSecToIncrement = (int)(_amtToSleep%1000000);
                if(!ranGetTimeOfDay)
                    gettimeofday(&now, 0);
                ts.tv_sec = now.tv_sec + nSecToIncrement;
                ts.tv_nsec = (now.tv_usec + nUSecToIncrement)*1000; //put in nsec

                pthread_mutex_lock(&_mutexToNominateToActiveList);
                if(_nominateToAddToActiveRunningList.empty())
                    pthread_cond_timedwait(&_condToNominateToActiveList, &_mutexToNominateToActiveList, &ts);
                pthread_mutex_unlock(&_mutexToNominateToActiveList);
            }
            else //sleep in non-threaded mode
                usleep(_amtToSleep);
        }
    }
    gettimeofday(&finish, 0);

    unsigned long long int diff = (finish.tv_sec-start.tv_sec)*1000000 + (finish.tv_usec - start.tv_usec);
    cerr<<pthread_self()<<" time taken in this thread = "<<diff<<"us"<<endl;
}


bool UFScheduler::_inThreadedMode = true;
UFScheduler* UFScheduler::getUFScheduler(pthread_t tid)
{
    if(!tid || tid == pthread_self())
        return (UFScheduler*)pthread_getspecific(_specific_key);

    pthread_mutex_lock(&_mutexToCheckFiberSchedulerMap);
    ThreadUFSchedulerMap::const_iterator index = _threadUFSchedulerMap.find(tid);
    if(index == _threadUFSchedulerMap.end())
    {
        pthread_mutex_unlock(&_mutexToCheckFiberSchedulerMap);
        return 0;
    }
    pthread_mutex_unlock(&_mutexToCheckFiberSchedulerMap);

    return const_cast<UFScheduler*>(index->second);
}

UF* UFScheduler::getUF(pthread_t tid)
{
    return const_cast<UF*>(getUFScheduler(tid)->getRunningFiberOnThisThread());
}

UFFactory::UFFactory()
{
    _size = 0;
    _capacity = 0;
    _objMapping = 0;
}

int UFFactory::registerFunc(UF* uf)
{
    //not making this code thread safe - since this should only happen at init time
    if(_size == _capacity)
    {
        _capacity  = _capacity ? _capacity : 5 /*start w/ 5 slots*/;
        _capacity *= 2; //double each time
        UF** tmpObjMapping = (UF**) malloc (sizeof(UF*)*_capacity);

        for(unsigned int i = 0; i < _size; ++i)
            tmpObjMapping[i] = _objMapping[i];
        if(_objMapping)
            free(_objMapping);

        _objMapping = tmpObjMapping;
    }

    _objMapping[_size] = uf;
    return _size++;
}

const unsigned int CONSECUTIVE_LOCK_FAILURES_ALLOWED = 3;
bool UFMutex::lock(UF* uf)
{
    if(!uf || !uf->_parentScheduler)
        return false;

    getSpinLock();
    if(_listOfClientsWaitingOnLock.empty()) //probably the most common case (no UF has the lock)
    {
#ifdef LOCK_DEBUG
        printf("%lu l1\n", (unsigned long int) ((uintptr_t)(void*)uf));
#endif
        _listOfClientsWaitingOnLock.push_back(uf);
        _lockCurrentlyOwned = true;
        releaseSpinLock();
        return true;
    }

    //see if any UF is holding the lock right now - if not get the lock
    //this is the case where between the time that an UF is woken up
    //(after another UF releases the lock)
    //and it actually runs this requesting UF might be able to procure the lock
    //if there is a mustRunUF - that UF has to run first - and this UF has to go to the end of the line
    if(!_lockCurrentlyOwned && !_mustRunUF)
    {
#ifdef LOCK_DEBUG
        printf("%lu l2\n", (unsigned long int) ((uintptr_t)(void*)uf));
#endif
        _listOfClientsWaitingOnLock.push_front(uf);
        _lockCurrentlyOwned = true;
        releaseSpinLock();
        return true;
    }

    //for the rest of the UFs that didnt meet the above criteria 
    //and didnt get the lock they have to wait
    _listOfClientsWaitingOnLock.push_back(uf);
    releaseSpinLock();

    unsigned short int counter = 0;
    while(1) //try to get the lock
    {
#ifdef LOCK_DEBUG
        printf("%lu wt\n", (unsigned long int) ((uintptr_t)(void*)uf));
#endif
        //simply yield - since the uf will be woken up once it gets the lock
        uf->waitOnLock();

        //since this uf got woken up - check if it can get the lock now
        getSpinLock();

        //check if any other UF has gotten the lock between the time that this UF 
        //got the notification and actually acted on it
        if(!_lockCurrentlyOwned && (_listOfClientsWaitingOnLock.front() == uf))
        {
#ifdef LOCK_DEBUG
            printf("%lu l3\n", (unsigned long int) ((uintptr_t)(void*)uf));
#endif
            _lockCurrentlyOwned = true;
            _mustRunUF = 0;
            releaseSpinLock();
            return true;
        }

        if(++counter >= CONSECUTIVE_LOCK_FAILURES_ALLOWED) //dont let a UF fail to get the lock more than CONSECUTIVE_LOCK_FAILURES_ALLOWED times
            _mustRunUF = uf;
        releaseSpinLock();
    }

    return true;
}

bool UFMutex::unlock(UF* uf)
{
    if(!uf)
        return false;

    UFList::iterator beg;
    getSpinLock();

    beg = _listOfClientsWaitingOnLock.begin();
    if(uf == *beg) //check if this uf is the current owner of this lock
    {
        _lockCurrentlyOwned = false;
        beg = _listOfClientsWaitingOnLock.erase(beg);
#ifdef LOCK_DEBUG
        printf("%lu u %d\n", (unsigned long int) ((uintptr_t)(void*)uf), _listOfClientsWaitingOnLock.size());
#endif

        bool releasedLock = false;
        //notify the next UF in line
        while(!_listOfClientsWaitingOnLock.empty())
        {
            UF* tmpUf = *beg;
            if(!tmpUf || !tmpUf->_parentScheduler) //invalid tmpuf - cant wake it up
            {
#ifdef LOCK_DEBUG
                printf("%lu nf1\n", (unsigned long int) ((uintptr_t)(void*)uf));
#endif
                beg = _listOfClientsWaitingOnLock.erase(beg);
                if(beg == _listOfClientsWaitingOnLock.end())
                    break;
                continue;
            }
            /*
            if(tmpUf->getStatus() == WAITING_TO_RUN) //this uf has already been put into the waiting to run list
                break;
                */


#ifdef LOCK_DEBUG
            printf("%lu wk %lu\n", 
                   (unsigned long int) ((uintptr_t)(void*)uf), 
                   (unsigned long int) ((uintptr_t)(void*)tmpUf));
#endif

            releaseSpinLock();
            releasedLock = true;
            uf->_parentScheduler->addFiberToScheduler(tmpUf, tmpUf->_parentScheduler->_tid);
            break;
        }

        if(!releasedLock)
            releaseSpinLock();

        return true;
    }
    else
    {
        cerr<<uf<<" tried to unlock but was not in top of list"<<endl;
        abort();
    }

    releaseSpinLock();
    return false;
}

bool UFMutex::tryLock(UF* uf, unsigned long long int autoRetryIntervalInUS)
{
    while(1)
    {
        getSpinLock();
        if(_listOfClientsWaitingOnLock.empty())
        {
            _listOfClientsWaitingOnLock.push_back(uf);
            _lockCurrentlyOwned = true;
            releaseSpinLock();
            return true;
        }

        releaseSpinLock();

        if(!autoRetryIntervalInUS)
            break;

        usleep(autoRetryIntervalInUS);
    }

    return false;
}


bool UFMutex::condWait(UF* uf)
{
    if(!uf)
        return false;
    
    //the object is already in the hash
    if(_listOfClientsWaitingOnCond.find(uf) == _listOfClientsWaitingOnCond.end())
    {
        UFWaitInfo *ufwi = uf->_parentScheduler->getWaitInfo();
        ufwi->_uf = uf;
        ufwi->_waiting = true;
    
        _listOfClientsWaitingOnCond[uf] = ufwi;
    }

    unlock(uf);
    uf->waitOnLock(); //this fxn will cause the fxn to wait till a signal or broadcast has occurred
    lock(uf);

    return true;
}

void UFMutex::broadcast()
{
    if(_listOfClientsWaitingOnCond.empty())
        return;

    UFScheduler* ufs = UFScheduler::getUFScheduler();
    if(!ufs)
    {
        cerr<<"couldnt get scheduler on thread "<<pthread_self()<<endl;
        return;
    }

    //notify all the UFs waiting to wake up
    bool sleeping = false;
    for(UFWLHash::iterator beg = _listOfClientsWaitingOnCond.begin();
        beg != _listOfClientsWaitingOnCond.end(); ++beg)
    {
        // Get WaitInfo object
        UFWaitInfo *ufwi = beg->second;

        ufwi->_ctrl.getSpinLock();
        ufwi->_waiting = false; // Set _waiting to false, indicating that the UFWI has been removed from the cond queue

        // If uf is not NULL, schedule it and make sure no one else can schedule it again
        if(ufwi->_uf) 
        {
            ufs->addFiberToScheduler(ufwi->_uf, ufwi->_uf->_parentScheduler->_tid);
            ufwi->_uf = NULL;
        }
        
        sleeping = ufwi->_sleeping;
        ufwi->_ctrl.releaseSpinLock();
        if(!sleeping) //sleep list has already run
            ufs->releaseWaitInfo(*ufwi);
    }
    _listOfClientsWaitingOnCond.clear();
}

void UFMutex::signal()
{
    if(_listOfClientsWaitingOnCond.empty())
        return;

    UFScheduler* ufs = UFScheduler::getUFScheduler();
    if(!ufs)
    {
        cerr<<"couldnt get scheduler"<<endl;
        return;
    }
    UF *uf_to_signal = NULL;
    bool sleeping = false;
    for(UFWLHash::iterator beg = _listOfClientsWaitingOnCond.begin(); beg != _listOfClientsWaitingOnCond.end();)
    {
        // Take first client off list
        UFWaitInfo *ufwi = beg->second;

        ufwi->_ctrl.getSpinLock();
        ufwi->_waiting = false; // Set _waiting to false, indicating that the UFWI has been removed from the cond queue
        
        if(ufwi->_uf)
        {
            uf_to_signal = ufwi->_uf; // Store UF to signal
            ufwi->_uf = NULL; // Clear UF. This ensures that no one else can schedule the UF.
        }

        sleeping = ufwi->_sleeping;
        ufwi->_ctrl.releaseSpinLock();
        if(!sleeping) //sleep list has already run
            ufs->releaseWaitInfo(*ufwi);

        // If a UF was found to signal, break out
        _listOfClientsWaitingOnCond.erase(beg);
        if(uf_to_signal)
            break;
        beg = _listOfClientsWaitingOnCond.begin();
    }

    if(uf_to_signal)
        ufs->addFiberToScheduler(uf_to_signal, uf_to_signal->_parentScheduler->_tid);
}

int UFMutex::condTimedWait(UF* uf, unsigned long long int sleepAmtInUs)
{
    bool result = false;
    if(!uf)
        return result;

    // Wrap uf in UFWait structure before pushing to wait and sleep queues
    
    UFWaitInfo *ufwi = UFScheduler::getUFScheduler()->getWaitInfo();
    ufwi->_uf = uf;
    ufwi->_waiting = true;
    ufwi->_sleeping = true;
    
    // Add to waiting queue
    _listOfClientsWaitingOnCond[uf] = ufwi;
    unlock(uf);
    
    // Add to sleep queue
    struct timeval now;
    gettimeofday(&now, 0);
    unsigned long long int timeNow = timeInUS(now);
    ufwi->_sleeping = true;
    uf->_parentScheduler->_sleepList.insert(std::make_pair((timeNow+sleepAmtInUs), ufwi));
    
    uf->waitOnLock(); //this fxn will cause the fxn to wait till a signal, broadcast or timeout has occurred

    ufwi->_ctrl.getSpinLock();
    result = ufwi->_sleeping;
    ufwi->_ctrl.releaseSpinLock();

    lock(uf);
    return (result) ? true : false;//if result (ufwi->_sleeping) is not true, it must be that the sleep list activated this uf
}

void* setupThread(void* args)
{
    if(!args)
        return 0;

    list<UF*>* ufsToStartWith = (list<UF*>*) args;
    UFScheduler ufs;
    ufs.addFibersToScheduler(*ufsToStartWith, 0);
    delete ufsToStartWith;

    //run the scheduler
    ufs.runScheduler();

    return 0;
}

void UFScheduler::ufCreateThread(pthread_t* tid, list<UF*>* ufsToStartWith)
{
    //create the threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if(pthread_create(tid, &attr, setupThread, (void*)ufsToStartWith) != 0)
    {
        cerr<<"couldnt create thread "<<strerror(errno)<<endl;
        exit(1);
    }
}
