/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   raf_cmd.cc
   Author          : Mike Chowla

   Description:

   $Id: raf_cmd.cc,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2001-2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#include "rafencode.h"
#include "raf_cmd.h"
#include "sio_buffer.h"

static char* raf_cmd_default_val = NULL;
RafCmd::RafCmd() :
    DynArray<char*>(&raf_cmd_default_val, 10)
{
}

RafCmd::~RafCmd() {

    int len = length();
    for (int i = 0; i < len; i++) {
	free((*this)[i]);
    }
}

void RafCmd::clear() {
    
    int len = length();
    for (int i = 0; i < len; i++) {
	free((*this)[i]);
    }
    DynArray<char*>::clear();
}

void RafCmd::process_cmd(char* cmd, int len) {

    int i = 0;
    int done = 0;
    char* end = cmd + len;
    const char* lastp = NULL;
    while (cmd < end) {
	int clen = raf_decodelen(cmd, end - cmd, &lastp);
	char* decode_buf = (char*) malloc(clen+1);

	int r = raf_decode(cmd, end - cmd, decode_buf, clen, &lastp);
	decode_buf[r] = '\0';
	if (r > 0) {
	    if (decode_buf[r-1] == '\r') {
		decode_buf[r-1] = '\0';
	    }
	}
	done += r;

	(*this)(i) = decode_buf;
	i++;
	cmd = (char*)lastp + 1;
    }
}

int RafCmd::build_message(sio_buffer* output_buffer) {

    int bytes_added = 0;
    int num_el = this->length();
    for (int i = 0; i < num_el; i++) {
	char* raw = (*this)[i];
	int raw_len = strlen(raw);
    	int enc_len = raf_encodelen(raw, raw_len, RAF_DISPLAY);

	output_buffer->expand_to(enc_len);
	enc_len = raf_encode(raw, raw_len, output_buffer->end(), enc_len, RAF_DISPLAY);
	output_buffer->fill(enc_len);

	if (i + 1 == num_el) {
	    output_buffer->fill("\n", 1);
	} else {
	    output_buffer->fill(" ", 1);
	}

	bytes_added += enc_len + 1; 
    }

    return bytes_added;
}
