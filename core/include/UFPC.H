#ifndef UFPC_H
#define UFPC_H

#include <set>
#include "UF.H"

struct UFMutex;
struct UFProducerData;
struct UFConsumer;
struct UFProducer;

enum UFProducerDataCode
{
    ADD = 1,
    END = 2
};

struct UFConsumer
{
    UFConsumer(bool shouldLockForInternalMods = true);
    virtual ~UFConsumer();
    //user needs to call input->releaseMe after consuming the data
    UFProducerData* waitForData(UF* uf = 0);
    bool hasData(UF* uf = 0);

    bool joinProducer(UFProducer* ufp);
    bool removeProducer(UFProducer* ufp);

    std::list<UFProducerData*>      _queueOfDataToConsume;
    UFMutex                         _queueOfDataToConsumeLock;

protected:
    std::set<UFProducer*>           _consumersProducerSet;
    UFMutex                         _consumersProducerSetLock;
    bool                            _shouldLockForInternalMods;
};

struct UFProducer
{
    friend class UFConsumer;
    UFProducer();
    ~UFProducer();
    bool removeConsumer(UFConsumer* ufc);
    bool produceData(void* data, size_t size, bool freeDataOnExit = true, UF* uf = 0);

protected:
    std::set<UFConsumer*>           _producersConsumerSet;
    UFMutex                         _producersConsumerSetLock; //needed when the consumers are adding or removing themselves from the consumerList

    /*TODO: producer can keep a collection of the data
    std::list<UFProducerData*>      _producerData;
    UFMutex                         _producerDataMutex; //this mutex is needed to add to the producerdata
    */
    bool addConsumer(UFConsumer* ufc);
};

struct UFProducerData
{
    void*                           _data;
    size_t                          _size;
    UFProducer*                     _producerWhichInserted;
    UFProducerDataCode              _ufpcCode;
    bool                            _freeDataOnExit;

    void addRef(size_t numToAdd = 1);
    void reduceRef();

    static UFProducerData* getObj();
    static void releaseObj(UFProducerData* obj);


protected:
    ~UFProducerData();
    UFProducerData() { _referenceCount = 0; _freeDataOnExit = true; }
    UFMutex                         _controlReferenceCount; //control the ref. count of this data
    size_t                          _referenceCount;
};

inline bool UFProducer::addConsumer(UFConsumer* ufc)
{
    _producersConsumerSetLock.getSpinLock();
    _producersConsumerSet.insert(ufc); //check insertion
    _producersConsumerSetLock.releaseSpinLock();
    return true;
}

inline bool UFProducer::removeConsumer(UFConsumer* ufc)
{
    _producersConsumerSetLock.getSpinLock();
    _producersConsumerSet.erase(ufc);
    _producersConsumerSetLock.releaseSpinLock();
    return true;
}

inline void UFProducerData::addRef(size_t numToAdd)
{
    _controlReferenceCount.getSpinLock();
    _referenceCount = numToAdd;
    _controlReferenceCount.releaseSpinLock();
}

inline void UFProducerData::reduceRef()
{
    _controlReferenceCount.getSpinLock();
    --_referenceCount;
    _controlReferenceCount.releaseSpinLock();
}

inline UFProducerData* UFProducerData::getObj()
{
    return new UFProducerData();
}

inline void UFProducerData::releaseObj(UFProducerData* obj)
{
    if(!obj)
        return;

    obj->_controlReferenceCount.getSpinLock();
    if(!--obj->_referenceCount)
        delete obj;
    else
        obj->_controlReferenceCount.releaseSpinLock();
}

inline UFProducerData::~UFProducerData()
{
    if(_freeDataOnExit && _data)
        free (_data);
}

#endif
