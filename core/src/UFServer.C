#include <iostream>
#include <errno.h>
#include <string.h>
#include "UFServer.H"
#include "UFServer.H"

#include <sys/types.h>
#include <sys/wait.h>
#include "UFStatSystem.H"
#include "UFStats.H"

using namespace std;

//TODO: handle signals later
//TODO: create monitoring port later
//
//
static string getPrintableTime()
{
    char asctimeDate[32];
    asctimeDate[0] = '\0';
    time_t now = time(0);
    asctime_r(localtime(&now), asctimeDate);

    string response = asctimeDate;
    size_t loc = response.find('\n');
    if(loc != string::npos)
        response.replace(loc, 1, "");
    return response;
}

void UFServer::reset()
{
    _addressToBindTo = "0";
    _listenFd = -1;
    _port = 0;
    _creationTime = 0;

    MAX_THREADS_ALLOWED = 8;
    MAX_PROCESSES_ALLOWED = 1;
    MAX_ACCEPT_THREADS_ALLOWED = 1;
    UF_STACK_SIZE = 8192;

    _threadChooser = 0;
}

UFServer::UFServer()
{
    reset();
}

struct NewConnUF : public UF
{
    void run()
    {
        if(!_startingArgs)
            return;

        UFIOAcceptArgs* fiberStartingArgs = (UFIOAcceptArgs*) _startingArgs;
        ((UFServer*) fiberStartingArgs->args)->handleNewConnection(fiberStartingArgs->ufio);
        // increment connections handled stat
        UFStatSystem::increment(UFStats::connectionsHandled);

        //clear the client connection
        delete fiberStartingArgs->ufio;
        //clear the arguments
        delete fiberStartingArgs;
        //the UF itself will be cleared by the scheduler
    }
    NewConnUF(bool registerMe = false)
    {
        if(registerMe)
            _myLoc = UFFactory::getInstance()->registerFunc((UF*)this);
    }
    UF* createUF() { return new NewConnUF(); }
    static NewConnUF* _self;
    static int _myLoc;
};
int NewConnUF::_myLoc = -1;
NewConnUF* NewConnUF::_self = new NewConnUF(true);

struct AcceptRunner : public UF
{
    void run()
    {
        if(!_startingArgs)
            return;
        UFServer* ufserver = (UFServer*) _startingArgs;

        //add the scheduler for this 
        UFIO* ufio = new UFIO(UFScheduler::getUF());
        int fd = ufserver->getListenFd();
        if(fd == -1)
            fd = UFIO::setupConnectionToAccept(ufserver->getBindingInterface(), ufserver->getPort() /*, deal w/ backlog*/);
        if(fd < 0)
        {
            cerr<<getPrintableTime()<<" "<<getpid()<<":couldnt setup listen socket"<<endl;
            exit(1);
        }
        if(!ufio || !ufio->setFd(fd, false/*has already been made non-blocking*/))
        {
            cerr<<getPrintableTime()<<" "<<getpid()<<":couldnt setup accept thread"<<endl;
            return;
        }

        ufio->accept(ufserver->_threadChooser, NewConnUF::_myLoc, ufserver, 0, 0);
    }
    AcceptRunner(bool registerMe = false)
    {
        if(registerMe)
            _myLoc = UFFactory::getInstance()->registerFunc((UF*)this);
    }
    UF* createUF() { return new AcceptRunner(); }
    static AcceptRunner* _self;
    static int _myLoc;
};
int AcceptRunner::_myLoc = -1;
AcceptRunner* AcceptRunner::_self = new AcceptRunner(true);

void UFServer::startThreads()
{
    preThreadCreation();

    MAX_THREADS_ALLOWED = (MAX_THREADS_ALLOWED ? MAX_THREADS_ALLOWED : 1);
    MAX_ACCEPT_THREADS_ALLOWED = (MAX_ACCEPT_THREADS_ALLOWED ? MAX_ACCEPT_THREADS_ALLOWED : 1);

    unsigned int i = 0;
    pthread_t* thread = new pthread_t[MAX_THREADS_ALLOWED+MAX_ACCEPT_THREADS_ALLOWED];
    //start the IO threads
    for(; i<MAX_THREADS_ALLOWED; i++)
    {
        UFIO::ufCreateThreadWithIO(&(thread[i]), new list<UF*>());
        cerr<<getPrintableTime()<<" "<<getpid()<<": created thread (with I/O) - "<<thread[i]<<endl;
        usleep(5000); //TODO: avoid the need for threadChooser to have a mutex - change to cond. var later
        //add the io threads to the thread chooser
        UFScheduler* ufs = UFScheduler::getUFScheduler(thread[i]);
        if(!ufs)
        {
            cerr<<getPrintableTime()<<" "<<getpid()<<": didnt get scheduler for tid - "<<thread[i]<<endl;
            exit(1);
        }
        addThread("NETIO", ufs, thread[i]);
    }

    //start the stats thread
    UFStatSystem::init(this);

    // Register server stats
    UFStats::registerStats();

    preAccept();
    //start the accept thread
    for(; i<MAX_ACCEPT_THREADS_ALLOWED+MAX_THREADS_ALLOWED; i++)
    {
        AcceptRunner* ar = new AcceptRunner();
        ar->_startingArgs = this;
        list<UF*>* ufsToAdd = new list<UF*>();
        ufsToAdd->push_back(ar);
        UFIO::ufCreateThreadWithIO(&(thread[i]), ufsToAdd);
        usleep(5000); //TODO: let the thread finish initializing 
        addThread("ACCEPT", 0, thread[i]);
        cerr<<getPrintableTime()<<" "<<getpid()<<": created accept thread (with I/O) - "<<thread[i]<<endl;
    }


    //wait for the threads to finish
    void* status;
    for(i=0; i<MAX_THREADS_ALLOWED+MAX_ACCEPT_THREADS_ALLOWED; i++)
        pthread_join(thread[i], &status);

    delete [] thread;
}

void UFServer::run()
{
    preForkRun();

    if(!_threadChooser)
        _threadChooser = new UFServerThreadChooser();

    //bind to the socket (before the fork
    _listenFd = UFIO::setupConnectionToAccept(_addressToBindTo.c_str(), _port); //TODO:set the backlog
    if(_listenFd < 0)
    {
        cerr<<getPrintableTime()<<" "<<getpid()<<": couldnt setup listen socket "<<strerror(errno)<<endl;
        exit(1);
    }

    if(!MAX_PROCESSES_ALLOWED) //an option to easily debug processes (or to only run in threaded mode)
    {
        preThreadRun();
        startThreads();
        return;
    }

    //fork children
    while(1)
    {
        while (getProcessCount() < MAX_PROCESSES_ALLOWED)
        {
            preBetweenFork();
            unsigned int pid = fork();
            if(pid < 0)
            {
                cerr<<getPrintableTime()<<" "<<getpid()<<": (P): couldnt create child# : "<<strerror(errno)<<endl;
                exit(1);
            }
            if(!pid) //child listens to conns
            {
                //TODO: DAEMONIZE LATER
                _creationTime = time(0);

                //now start
                postForkPreRun();
                preThreadRun();
                startThreads();
                exit(0);
            }
            cerr<<getPrintableTime()<<" "<<getpid()<<": (P): started child process: "<<pid<<endl;
            _childProcesses[pid] = time(0);
            postBetweenFork(pid);
        }


        int child_exit_status;
        int child_pid = waitpid(-1, &child_exit_status, WNOHANG);
        if(child_pid == 0) { }
        else if(child_pid < 0)
        {
            if(errno != ECHILD)
                cerr<<getPrintableTime()<<" "<<getpid()<<": (P): waitpid error:"<<endl;
        }
        else if(child_pid > 0)
        {
            cerr<<getPrintableTime()<<" "<<getpid()<<")(P): child_pid "<<child_pid<<" died "<<endl;
            map<int, time_t>::iterator itr = _childProcesses.find(child_pid);
            if(itr != _childProcesses.end()) 
                _childProcesses.erase(itr);
        }

        //we've been asked to bail
        if(!MAX_PROCESSES_ALLOWED && !_childProcesses.size())
            break;

        //let the parent rest
        usleep(500000);
    }
}

void UFServer::addThread(const std::string& type, UFScheduler* ufScheduler, pthread_t tid)
{
    if(!tid)
        tid = pthread_self();

    StringThreadMapping::iterator index = _threadList.find(type);
    if(index == _threadList.end())
    {
        _threadList[type] = new std::vector<pthread_t>;
        index = _threadList.find(type);
        if(index == _threadList.end())
            return;
    }
    index->second->push_back(tid);

    if(ufScheduler && type == "NETIO")
        _threadChooser->add(ufScheduler, tid);
}

vector<pthread_t>* UFServer::getThreadType(const string& type)
{ 
    StringThreadMapping::iterator index = _threadList.find(type);
    if(index == _threadList.end())
        return 0;

    return index->second;
}


