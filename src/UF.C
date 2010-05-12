#include "UF.H"
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
pthread_mutex_t UFScheduler::_mutexToCheckFiberScheduerMap = PTHREAD_MUTEX_INITIALIZER;
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

        pthread_mutex_lock(&_mutexToCheckFiberScheduerMap);
        if(_threadUFSchedulerMap.find(currThreadId) != _threadUFSchedulerMap.end())
        {
            cerr<<"cannot have more than one scheduler per thread"<<endl;
            exit(1);
        }
        _threadUFSchedulerMap[currThreadId] = this;
        pthread_mutex_unlock(&_mutexToCheckFiberScheduerMap);
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
}

UFScheduler::~UFScheduler()
{
    pthread_key_delete(_specific_key);
}


bool UFScheduler::addFiberToScheduler(UF* uf, pthread_t tid)
{
    if(!uf)
    {
        cerr<<"returning cause there is a scheduler already"<<endl;
        return false;
    }

    list<UF*> ufList;
    ufList.push_front(uf);
    return addFibersToScheduler(ufList, tid);
}

bool UFScheduler::addFibersToScheduler(const list<UF*>& ufList, pthread_t tid)
{
    if(ufList.empty())
        return true;

    list<UF*>::const_iterator beg = ufList.begin();
    list<UF*>::const_iterator ending = ufList.end();
    //adding to the same scheduler and as a result thread as the current job
    if(!tid || (tid == pthread_self()))
    {
        for(; beg != ending; ++beg)
        {
            UF* uf = *beg;
            uf->_status = WAITING_TO_RUN;
            if(uf->_parentScheduler) //probably putting back an existing uf into the active list
            {
                if(uf->_parentScheduler != this) //cant schedule for some other thread
                {
                    cerr<<"uf is not part of this scheduler"<<endl;
                    return false;
                }
                _activeRunningList.push_back(uf);
                continue;
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
            _activeRunningList.push_back(uf);
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
            ufs->_nominateToAddToActiveRunningList.push_back(*beg);
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

    unsigned long long int amtToSleep = DEFAULT_SLEEP_IN_USEC;
    bool ranGetTimeOfDay = false;
    bool firstRun = true;

    UFList::iterator beg;
    struct timeval now;
    struct timeval start,finish;
    gettimeofday(&start, 0);
    while(!_exitJustMe && !_exit)
    {
        UFList::iterator beg = _activeRunningList.begin();
        for(; beg != _activeRunningList.end(); )
        {
            UF* uf = *beg;
            _currentFiber = uf;
            uf->_status = RUNNING;
            swapcontext(&_mainContext, &(uf->_UFContext));
            _currentFiber = 0;

            if(uf->_status == RUNNING) { }
            else if(uf->_status == BLOCKED)
            {
                beg = _activeRunningList.erase(beg);
                continue;
            }
            else if(uf->_status == COMPLETED) 
            {
                delete uf;
                beg = _activeRunningList.erase(beg);
                continue;
            }

            uf->_status = WAITING_TO_RUN;
            ++beg;
        }


        //check if some other thread has nominated some user fiber to be
        //added to this thread's list -
        //can happen in the foll. situations
        //1. the main thread is adding a new user fiber
        //2. some fiber has requested to move to another thread
        if(!_nominateToAddToActiveRunningList.empty() /*TODO: take this out later w/ the atomic size count*/ &&
           _inThreadedMode)

        {
            //TODO: do atomic comparison to see if there is anything in 
            //_nominateToAddToActiveRunningList before getting the lock
            pthread_mutex_lock(&_mutexToNominateToActiveList);
            UFList::iterator beg = _nominateToAddToActiveRunningList.begin();
            for(; beg != _nominateToAddToActiveRunningList.end(); )
            {
                UF* uf = *beg;
                if(uf->_parentScheduler)
                    _activeRunningList.push_back(uf);
                else //adding a new fiber
                    addFiberToScheduler(uf, 0);
                beg = _nominateToAddToActiveRunningList.erase(beg);
            }

            pthread_mutex_unlock(&_mutexToNominateToActiveList);
        }


        ranGetTimeOfDay = false;
        amtToSleep = DEFAULT_SLEEP_IN_USEC;
        //pick up the fibers that may have completed sleeping
        //look into the sleep list;
        if(!_sleepList.empty())
        {
            gettimeofday(&now, 0);
            ranGetTimeOfDay = true;
            unsigned long long int timeNow = (now.tv_sec*1000000)+now.tv_usec;
            firstRun = true;
            for(MapTimeUF::iterator beg = _sleepList.begin(); beg != _sleepList.end(); )
            {
                //TODO: has to be cleaned up
                //1. see if anyone has crossed the sleep timer - add them to the active list
                if(beg->first <= timeNow) //sleep time is over
                {
                    _activeRunningList.push_back(beg->second);
                    _sleepList.erase(beg);
                    beg = _sleepList.begin();
                    continue;
                }
                else
                {
                    if(firstRun)
                        amtToSleep = beg->first-timeNow;
                    break;
                }
                firstRun = false;
                ++beg;
            }
        }

        //see if there is anything to do or is it just sleeping time now
        if(!_notifyFunc && _activeRunningList.empty() && !_exit)
        {
            if(_inThreadedMode) //go to conditional wait (in threaded mode)
            {
                struct timespec ts;
                unsigned long long int nSecToIncrement = (int)(amtToSleep/1000000);
                unsigned long long int nUSecToIncrement = (int)(amtToSleep%1000000);
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
                usleep(amtToSleep);
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

    pthread_mutex_lock(&_mutexToCheckFiberScheduerMap);
    ThreadUFSchedulerMap::const_iterator index = _threadUFSchedulerMap.find(tid);
    if(index == _threadUFSchedulerMap.end())
    {
        pthread_mutex_unlock(&_mutexToCheckFiberScheduerMap);
        return 0;
    }
    pthread_mutex_unlock(&_mutexToCheckFiberScheduerMap);

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

void* setupThread(void* args)
{
    if(!args)
        return 0;

    list<UF*>* ufsToStartWith = (list<UF*>*) args;
    UFScheduler ufs;
    for(list<UF*>::iterator beg = ufsToStartWith->begin();
        beg != ufsToStartWith->end();
        ++beg)
        ufs.addFiberToScheduler(*beg);
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
