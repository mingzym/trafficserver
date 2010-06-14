#ifndef UFPC_H
#define UFPC_H

#include <set>
#include <stack>
#include "UF.H"

struct UFMutex;
struct UFProducerData;
struct UFConsumer;
struct UFProducer;

struct UFConsumer
{
    friend class UFProducer;
    UFConsumer(bool notifyOnExitOnly = false);
    virtual ~UFConsumer();
    //user needs to call input->releaseMe after consuming the data
    UFProducerData* waitForData(UF* uf = 0);
    bool hasData(UF* uf = 0);

    bool joinProducer(UFProducer* ufp);
    bool removeProducer(UFProducer* ufp);
    void reset();

    std::list<UFProducerData*>      _queueOfDataToConsume;
    UFMutex                         _queueOfDataToConsumeLock;
    bool getNotifyOnExitOnly() const;

    UF* getUF() const;
    bool                            _requireLockToWaitForUpdate; //if the developer is aware that both the producer and all the consumers are going to run in the same thread - only then set this variable to false to gain some perf. benefits
protected:
    std::set<UFProducer*>           _consumersProducerSet;
    UFMutex                         _consumersProducerSetLock;
    bool                            _notifyOnExitOnly;
    UF*                             _currUF;
};
inline UF* UFConsumer::getUF() const { return _currUF; }
inline bool UFConsumer::getNotifyOnExitOnly() const { return _notifyOnExitOnly; }

struct UFProducer
{
    friend class UFConsumer;
    UFProducer();
    virtual ~UFProducer();
    bool removeConsumer(UFConsumer* ufc);
    bool produceData(void* data, unsigned int size, int ufpcCode, bool freeDataOnExit = true, UF* uf = 0);
    void reset();
    void init();
    bool                            _requireLockToUpdateConsumers;//if the developer is aware that both the producer and the consumers are going to run in the same thread - only then set this variable to false to gain some perf. benefits

protected:
    std::set<UFConsumer*>           _producersConsumerSet;
    size_t                          _producersConsumerSetSize;
    UFConsumer*                     _mostRecentConsumerAdded;
    UFMutex                         _producersConsumerSetLock; //needed when the consumers are adding or removing themselves from the consumerList
    bool                            _acceptNewConsumers;

    bool addConsumer(UFConsumer* ufc);
    UF*                             _uf;
};

struct UFProducerData
{
    void*                           _data;
    unsigned int                    _size;
    UFProducer*                     _producerWhichInserted;
    int                             _ufpcCode;
    bool                            _freeDataOnExit;
    bool                            _lockToUpdate;

    void addRef(size_t numToAdd = 1);
    void reduceRef();
    void reset();

    static UFProducerData* getObj();
    static void releaseObj(UFProducerData* obj);


protected:
    ~UFProducerData();
    UFProducerData() { reset(); }
    UFMutex                         _controlReferenceCount; //control the ref. count of this data
    size_t                          _referenceCount;
    static std::stack<UFProducerData*> _objList;
    static UFMutex                  _objListMutex;
};
inline void UFProducerData::reset()
{
    _referenceCount = 0; 
    _freeDataOnExit = true; 
    _lockToUpdate = true;
}

inline bool UFProducer::addConsumer(UFConsumer* ufc)
{
    UF* uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.lock(uf);
    if(!_acceptNewConsumers)
    {
        if(_requireLockToUpdateConsumers) _producersConsumerSetLock.unlock(uf);
        return false;
    }
    _producersConsumerSet.insert(ufc); //check insertion
    _producersConsumerSetSize++;
    _mostRecentConsumerAdded = ufc;
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.unlock(uf);
    return true;
}

inline bool UFProducer::removeConsumer(UFConsumer* ufc)
{
    UF* uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.lock(uf);
    _producersConsumerSet.erase(ufc);
    _producersConsumerSetSize--;
    if(_requireLockToUpdateConsumers) 
        _producersConsumerSetLock.signal();
    else
    {
        if(_uf)
            UFScheduler::getUFScheduler()->addFiberToScheduler(_uf);
    }
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.unlock(uf);
    return true;
}

inline void UFProducerData::addRef(size_t numToAdd)
{
    if(_lockToUpdate) _controlReferenceCount.getSpinLock();
    _referenceCount = numToAdd;
    if(_lockToUpdate) _controlReferenceCount.releaseSpinLock();
}

inline void UFProducerData::reduceRef()
{
    if(_lockToUpdate) _controlReferenceCount.getSpinLock();
    --_referenceCount;
    if(_lockToUpdate) _controlReferenceCount.releaseSpinLock();
}

inline UFProducerData* UFProducerData::getObj()
{
    /*
    _objListMutex.getSpinLock();
    if(!_objList.empty())
    {
        UFProducerData* retVal = _objList.top();
        _objList.pop();
        _objListMutex.releaseSpinLock();
        retVal->reset();
        return retVal;
    }
    _objListMutex.releaseSpinLock();
    */
    return new UFProducerData();
}

inline void UFProducerData::releaseObj(UFProducerData* obj)
{
    if(!obj)
        return;

    if(obj->_lockToUpdate) obj->_controlReferenceCount.getSpinLock();
    if(!--obj->_referenceCount)
    {
        /*
        _objListMutex.getSpinLock();
        _objList.push(obj);
        _objListMutex.releaseSpinLock();
        */
        delete obj;
        return;
    }
    if(obj->_lockToUpdate) obj->_controlReferenceCount.releaseSpinLock();
}

inline UFProducerData::~UFProducerData()
{
    if(_freeDataOnExit && _data)
        free (_data);
}

inline UFConsumer::~UFConsumer() { reset(); } 

inline UFProducer::~UFProducer() { reset(); }
inline void UFProducer::init() 
{ 
    _acceptNewConsumers = true; 
    _requireLockToUpdateConsumers = true; 
    _producersConsumerSetSize = 0;
    _mostRecentConsumerAdded = 0;
}
inline UFProducer::UFProducer() { init(); }

#endif
