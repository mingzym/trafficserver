/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   sio_loop.h
   Author          : Mike Chowla

   Description:

   $Id: sio_loop.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2001-2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _SIO_LOOP_H_
#define _SIO_LOOP_H_

#include "List.h"
#include "ink_hrtime.h"

#ifndef Darwin
#include <poll.h>
#else
#include "ink_poll.h"
#endif

enum poll_interest_t {
    POLL_INTEREST_NONE = 0,
    POLL_INTEREST_READ = 1,
    POLL_INTEREST_WRITE = 2,
    POLL_INTEREST_RW = 3
};

enum s_event_t {
    SEVENT_NONE = 0,
    SEVENT_POLL = 1,
    SEVENT_TIMER = 2,
    SEVENT_PROC_STATE_CHANGE = 3,
    SEVENT_EXIT_NOTIFY = 4,

    SEVENT_RMDIR_SUCCESS = 1000,
    SEVENT_RMDIR_FAILURE = 1001
};

struct S_Continuation;
typedef void (S_Continuation::*SCont_Handler)(s_event_t, void* );

struct S_Continuation {
  public:
    S_Continuation();
    virtual ~S_Continuation();

    void handle_event(s_event_t, void* );
    SCont_Handler my_handler;
};

struct  FD_Handler : public S_Continuation {
  public:
    FD_Handler();
    virtual ~FD_Handler();

    int clear_non_block_flag();
    int set_linger(int on, int ltime);

    int fd;
    poll_interest_t poll_interest;

    Link<FD_Handler> link;
};

struct S_Action {
  public:
    S_Action();
    virtual ~S_Action();
    virtual void cancel();

    int cancelled;
    S_Continuation* s_cont;

    Link<S_Action> action_link;
};

struct S_Event : public S_Action {
  public:
    S_Event();
    virtual ~S_Event();

    ink_hrtime when;
    Link<S_Event> event_link;
};



class SIO {
  public:
    static void run_loop();
    static void run_loop_once();

    static int open_server(unsigned short int port);
    static int accept_sock(int sock);
    static int make_client (unsigned int addr, int port);

    static S_Event* schedule_in(S_Continuation* s, int ms);
    static void add_fd_handler(FD_Handler* );
    static void remove_fd_handler(FD_Handler* );
    static void add_exit_handler(S_Continuation* );
    static void do_exit(int status);

};

#endif
