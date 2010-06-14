#include "UFPC.H"
#include <stdio.h>

using namespace std;

bool UFProducer::produceData(void* data, unsigned int size, int ufpcCode, bool freeDataOnExit, UF* uf)
{
    if(!uf)
        uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    _uf = uf;

    //create the UFProducerData structure
    UFProducerData* ufpd = UFProducerData::getObj();
    ufpd->_data = data;
    ufpd->_size = size;
    ufpd->_freeDataOnExit = freeDataOnExit;
    ufpd->_producerWhichInserted = this;
    ufpd->_ufpcCode = ufpcCode;
    ufpd->_lockToUpdate = _requireLockToUpdateConsumers;

    //increase the reference count
    ufpd->addRef(_producersConsumerSetSize);

    //the optimized case of no locking + there being only one consumer
    if(!_requireLockToUpdateConsumers &&
       _producersConsumerSetSize == 1 &&
       _mostRecentConsumerAdded)
    {
        _mostRecentConsumerAdded->_queueOfDataToConsume.push_back(ufpd);
        UF* consUF = _mostRecentConsumerAdded->getUF();
        if(consUF)
            UFScheduler::getUFScheduler()->addFiberToScheduler(consUF);
        return true;
    }

    //for each of the consumers add it to their queue
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.lock(uf);
    for(set<UFConsumer*>::iterator beg = _producersConsumerSet.begin();
        beg != _producersConsumerSet.end(); ++beg)
    {
        //add to the consumer's queue
        if(_requireLockToUpdateConsumers)
        {
            (*beg)->_queueOfDataToConsumeLock.lock(uf);
            (*beg)->_queueOfDataToConsume.push_back(ufpd);
            if(!(*beg)->getNotifyOnExitOnly() || (ufpcCode == 0))
                (*beg)->_queueOfDataToConsumeLock.signal();
            (*beg)->_queueOfDataToConsumeLock.unlock(uf);
        }
        else
        {
            (*beg)->_queueOfDataToConsume.push_back(ufpd);
            UF* consUF = (*beg)->getUF();
            if(consUF && (!(*beg)->getNotifyOnExitOnly() || (ufpcCode == 0)))
                UFScheduler::getUFScheduler()->addFiberToScheduler(consUF);
        }
    }
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.unlock(uf);

    return true;
}

UFConsumer::UFConsumer(bool notifyOnExitOnly) : _notifyOnExitOnly(notifyOnExitOnly)
{ 
    _currUF = 0; 
    _requireLockToWaitForUpdate = true;
}

UFProducerData* UFConsumer::waitForData(UF* uf)
{
    if(!uf)
        uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();

    _currUF = uf;

    if(_requireLockToWaitForUpdate) _queueOfDataToConsumeLock.lock(uf);
    while(_queueOfDataToConsume.empty())
    {
        if(_requireLockToWaitForUpdate) _queueOfDataToConsumeLock.condWait(uf); //TODO: change to condTimedWait
        else uf->block(); //wait for the producer to wake up the consumer
    }

    //read the first element from the queue
    UFProducerData* result = _queueOfDataToConsume.front();
    _queueOfDataToConsume.pop_front();
    if(_requireLockToWaitForUpdate) _queueOfDataToConsumeLock.unlock(uf); //release the lock gotten earlier

    return result;
}

bool UFConsumer::hasData(UF* uf)
{
    if(!uf)
        uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();

    bool result;
    if(_requireLockToWaitForUpdate) _queueOfDataToConsumeLock.lock(uf);
    result = !(_queueOfDataToConsume.empty());
    if(_requireLockToWaitForUpdate) _queueOfDataToConsumeLock.unlock(uf); //release the lock gotten earlier

    return result;
}

bool UFConsumer::joinProducer(UFProducer* ufp)
{
    if(_requireLockToWaitForUpdate) _consumersProducerSetLock.getSpinLock();
    _consumersProducerSet.insert(ufp);
    if(_requireLockToWaitForUpdate) _consumersProducerSetLock.releaseSpinLock();

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

    if(_requireLockToWaitForUpdate) _consumersProducerSetLock.getSpinLock();
    _consumersProducerSet.erase(ufp);
    if(_requireLockToWaitForUpdate) _consumersProducerSetLock.releaseSpinLock();
    return true;
}

void UFConsumer::reset()
{
    //1. notify all the producers on exit
    for(set<UFProducer*>::iterator beg = _consumersProducerSet.begin(); beg != _consumersProducerSet.end(); )
    {
        removeProducer(*beg);
        beg = _consumersProducerSet.begin();
    }

    //2. clear out all the remaining entries in the queue
    if(!_queueOfDataToConsume.empty())
    {
        list<UFProducerData*>::iterator beg = _queueOfDataToConsume.begin();
        for(; beg != _queueOfDataToConsume.end(); ++beg)
            UFProducerData::releaseObj((*beg));
    }
}

void UFProducer::reset()
{
    //add the EOF indicator
    produceData(0, 0, 0/*exit*/, 0/*freeDataOnExit*/); //notify the consumers that the producer is bailing

    //have to wait for all the consumers to acknowledge my death
    UF* uf = UFScheduler::getUFScheduler()->getRunningFiberOnThisThread();
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.lock(uf);
    _acceptNewConsumers = false;
    while(!_producersConsumerSet.empty())
    {
        if(_requireLockToUpdateConsumers) _producersConsumerSetLock.condTimedWait(uf, 1000000);
        else 
        {
            _uf = uf;
            _uf->block();
        }
    }
    if(_requireLockToUpdateConsumers) _producersConsumerSetLock.unlock(uf);
}

stack<UFProducerData*> UFProducerData::_objList;
UFMutex UFProducerData::_objListMutex;
