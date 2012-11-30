/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   test_log_collate.h
   Author          : Mike Chowla

   Description:

   $Id: test_log_collate.h,v 1.2 2003-06-01 18:38:30 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _TEST_LOG_COLLATE_H_
#define _TEST_LOG_COLLATE_H_

#include "sio_loop.h"
#include "sio_buffer.h"
#include "sio_raf_server.h"

class LogAcceptHandler : public FD_Handler {
  public:
    LogAcceptHandler();
    ~LogAcceptHandler();

    void start(int port);
    void stop();
    void handle_accept(s_event_t, void* );
};

enum Log_Collator_Mode_t {
    LC_RAF,
    LC_COLLATE
};

class LogCollateHandler : public SioRafServer {
  public:
    LogCollateHandler();
    virtual ~LogCollateHandler();

    void LogCollateHandler::start(int new_fd);
    void handle_log_input(s_event_t, void*);
    void wait_for_shutdown_complete(s_event_t, void* );
    static int active_loggers;

  protected:
    virtual void dispatcher();
    virtual void response_complete();
    
  private:
    void process_cmd_shutdown();
    void process_cmd_log_roll();

    Log_Collator_Mode_t lc_mode;
    sio_buffer* input_buffer;

    // shutdown stuff
    S_Event* timer_event;
};


#endif
