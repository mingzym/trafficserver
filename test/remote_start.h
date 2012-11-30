/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   remote_start.h
   Author          : Mike Chowla

   Description:

   $Id: remote_start.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _REMOTE_START_H_
#define _REMOTE_START_H_

struct UserDirInfo;
int remote_start(const char* hostname, unsigned int ip, UserDirInfo* ud,
	             int remote_start_mgr_killtm);

#endif
