/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   sio_buffer.h
   Author          : Mike Chowla

   Description:
      A simple single reader io buffer which keeps data continguous
         by copying it

   $Id: sio_buffer.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2001 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _SIO_BUFFER_H_
#define _SIO_BUFFER_H_

#define DEFAULT_SIO_SIZE 2048

#include <limits.h>

class sio_buffer {
  public:
    sio_buffer();
    sio_buffer(int init_size);
    ~sio_buffer();

    // we make write_avail at least size
    int expand_to(int size);

    int fill(const char* data, int data_len);
    int fill(int n);

    int read_avail();
    int write_avail();

    char* start();
    char* end();

    void consume(int n);
    void reset();

    char* memchr(int c, int len = INT_MAX, int offset = 0);
  
    // consume data 
    int read(char * buf, int len); 
    
    // does not consume data
    char* memcpy(char * buf, int len = INT_MAX, int offset = 0); 

  private:

    void init_buffer(int size);

    // No gratuitous copies!
    sio_buffer (const sio_buffer &m);
    sio_buffer& operator = (const sio_buffer &m);    

    char* raw_start;
    char* raw_end;

    char* data_start;
    char* data_end;
};

#endif
