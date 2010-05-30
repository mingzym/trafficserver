#include "UFPC.H"
#include <stdio.h>

using namespace std;

bool UFProducer::produceData(void* data, size_t size, bool freeDataOnExit, UF* uf)
{
    if(!uf)
        uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();

    //create the UFProducerData structure
    UFProducerData* ufpd = UFProducerData::getObj();
    ufpd->_data = data;
    ufpd->_size = size;
    ufpd->_freeDataOnExit = freeDataOnExit;
    ufpd->_producerWhichInserted = this;
    ufpd->_ufpcCode = (data && size) ? ADD : END;


    /*TODO: add the structure to the queue 
    _producerDataMutex.lock(uf);
    _producerData.push_back(ufpd);
    _producerDataMutex.unlock(uf);
    */

    //for each of the consumers add it to their queue
    _producersConsumerSetLock.getSpinLock();
    //increase the reference count
    ufpd->addRef(_producersConsumerSet.size());
    for(set<UFConsumer*>::iterator beg = _producersConsumerSet.begin();
        beg != _producersConsumerSet.end(); ++beg)
    {
        //add to the consumer's queue
        (*beg)->_queueOfDataToConsumeLock.lock(uf);
        (*beg)->_queueOfDataToConsume.push_back(ufpd);
        (*beg)->_queueOfDataToConsumeLock.signal();
        (*beg)->_queueOfDataToConsumeLock.unlock(uf);
    }
    //printf("%lu added to %lu\n", (unsigned long int) ((uintptr_t)(void*)uf), (unsigned long int)_producersConsumerSet.size());
    _producersConsumerSetLock.releaseSpinLock();

    return true;
}

UFConsumer::UFConsumer(bool shouldLockForInternalMods) : _shouldLockForInternalMods(shouldLockForInternalMods) { }

UFProducerData* UFConsumer::waitForData(UF* uf)
{
    if(!uf)
        uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();

    _queueOfDataToConsumeLock.lock(uf);
    while(_queueOfDataToConsume.empty())
        _queueOfDataToConsumeLock.condWait(uf); //TODO: change to condTimedWait

    //read the first element from the queue
    UFProducerData* result = _queueOfDataToConsume.front();
    _queueOfDataToConsume.pop_front();
    //printf("%lu consumer read 1 out %lu\n", (unsigned long int) ((uintptr_t)(void*)uf), (unsigned long int)_queueOfDataToConsume.size());
    _queueOfDataToConsumeLock.unlock(uf); //release the lock gotten earlier

    return result;
}

bool UFConsumer::hasData(UF* uf)
{
    if(!uf)
        uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();

    bool result;
    _queueOfDataToConsumeLock.lock(uf);
    result = !(_queueOfDataToConsume.empty());
    _queueOfDataToConsumeLock.unlock(uf);

    return result;
}

bool UFConsumer::joinProducer(UFProducer* ufp)
{
    if(_shouldLockForInternalMods)
        _consumersProducerSetLock.getSpinLock();
    _consumersProducerSet.insert(ufp);
    if(_shouldLockForInternalMods)
        _consumersProducerSetLock.releaseSpinLock();

    //notify the producer that we're adding this consumer
    if(!ufp->addConsumer(this))
        return false;

    return true;
}

bool UFConsumer::removeProducer(UFProducer* ufp)
{
    //notifying producer on exit
    if(!ufp->removeConsumer(this))
        return false;

    if(_shouldLockForInternalMods)
        _consumersProducerSetLock.getSpinLock();
    _consumersProducerSet.erase(ufp);
    if(_shouldLockForInternalMods)
        _consumersProducerSetLock.releaseSpinLock();
    return true;
}

UFConsumer::~UFConsumer()
{
    //1. notify all the producers on exit
    for(set<UFProducer*>::iterator beg = _consumersProducerSet.begin();
        beg != _consumersProducerSet.end();
        ++beg)
        removeProducer(*beg);

    //2. clear out all the remaining entries in the queue
    if(!_queueOfDataToConsume.empty())
    {
        list<UFProducerData*>::iterator beg = _queueOfDataToConsume.begin();
        for(; beg != _queueOfDataToConsume.end(); ++beg)
            UFProducerData::releaseObj((*beg));
    }
} 

UFProducer::UFProducer()
{
}

UFProducer::~UFProducer()
{
    produceData(0, 0, 0); //notify the consumers that the producer is bailing

    //have to wait for all the consumers to acknowledge my death
    while(1)
    {
        //TODO: change to signal based later
        _producersConsumerSetLock.getSpinLock();
        if(!_producersConsumerSet.size())
            break;
        _producersConsumerSetLock.releaseSpinLock();
        usleep(100000);
    }
}

