/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   log_sender.h
   Author          : Mike Chowla

   Description:

   $Id: log_sender.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _LOG_SENDER_H_
#define _LOG_SENDER_H_

#include "sio_loop.h"
#include "sio_buffer.h"

class LogSender : public FD_Handler {
  public:
    LogSender();
    ~LogSender();

    void start_to_file(const char* filename);
    void start_to_net(unsigned int ip, int port);

    void handle_output(s_event_t, void*);
    void add_to_output_log(const char* start, const char* end);

    void flush_output();
    void close_output();

    const char* LogSender::roll_log_file(const char* roll_name);

  private:
    char* log_file_name;
    sio_buffer* output_log_buffer;
};

#endif



