/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   raf_cmd.h
   Author          : Mike Chowla

   Description:

   $Id: raf_cmd.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2001-2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _RAF_CMD_H_
#define _RAF_CMD_H_

#include "DynArray.h"

class sio_buffer;

class RafCmd : public DynArray<char*> {
  public:
    RafCmd();
    ~RafCmd();
    void process_cmd(char* cmd, int len);
    int build_message(sio_buffer* output_buffer);
    void clear ();
};



#endif
