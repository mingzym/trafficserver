/************* -*- Mode: C++; Indent: Inktomi4 -*- **************************

  rafencode.h

  Functions for escapifying and unescapfying for the RAF protocol

  Copied from SF src tree.  Was am-1/misc/rafencode.h v1.4

  Copyright 1999-2000 FastForward Networks, Inc. All Rights Reserved.
  (c) 2001 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _RAFENCODE_H
#define _RAFENCODE_H

/* This string will be displayed for a user to see.  Can generate a more
 * verbose string */
#define RAF_DISPLAY 1

#ifdef __cplusplus
extern "C" {
#endif
int raf_encodelen(const char *inbuf, int inlen, int flags);
int raf_encode(const char *inbuf, int inlen,
	       char *outbuf, int outlen, int flags);
int raf_decodelen(const char *inbuf, int inlen, const char **lastp);
int raf_decode(const char *inbuf, int inlen,
	       char *outbuf, int outlen, const char **lastp);
#ifdef __cplusplus
}
#endif

#endif /* _RAFENCODE_H */
