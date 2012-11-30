/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   sio_raf_server.h
   Author          : Mike Chowla

   Description:

   $Id: sio_raf_server.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2001-2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _SIO_RAF_SERVER_
#define _SIO_RAF_SERVER_

#include "sio_loop.h"

class sio_buffer;
class RafCmd;
class SioRafServer;

enum Raf_Exit_Mode_t {
    RAF_EXIT_NONE,
    RAF_EXIT_CONN,
    RAF_EXIT_PROCESS
};

class SioRafServer : public FD_Handler {
  public:
    SioRafServer();
    virtual ~SioRafServer();

    virtual void start(int new_fd);
    void process_cmd(char* end);

    void handle_read_cmd(s_event_t, void*);
    void handle_write_resp(s_event_t, void*);

  protected:
    virtual void dispatcher();
    virtual void response_complete();

    void send_raf_resp(RafCmd* reply);
    void send_raf_resp(RafCmd* cmd, int result_code, const char* msg_fmt, ...);

    RafCmd* raf_cmd;
    Raf_Exit_Mode_t exit_mode;
    sio_buffer* cmd_buffer;
    sio_buffer* resp_buffer;
};


#endif
