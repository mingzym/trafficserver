/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   test_utils.h
   Author          : Mike Chowla

   Description:

   $Id: test_utils.h,v 1.2 2003-06-01 18:38:30 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_

char** build_argv(const char* arg0, const char* rest, int* r_argc);
char** build_argv_v(const char* arg0, ...);
char** append_argv(char** argv1, char** argv2);

void destroy_argv(char** argv);

int check_package_file_extension(const char* file_name, const char** ext_ptr);
const char* create_or_verify_dir(const char* dir, int* error_code);

// Caller frees return value
char* get_arch_str();

class sio_buffer;

const char* write_buffer(int fd, sio_buffer* buf, int* timeout_ms);
const char* read_until(int fd, sio_buffer* read_buffer, char end_chr, int* timeout_ms);

const char* read_to_buffer(int fd, sio_buffer* read_buffer, int nbytes,
			   int* eof, int* timeout_ms);

class RafCmd;
const char* send_raf_cmd(int fd, RafCmd* request, int* timeout_ms);
const char* read_raf_resp(int fd, sio_buffer* read_buffer,
			  RafCmd* response, int* timeout_ms);

class FreeOnDestruct {
  public:
    FreeOnDestruct(void* p);
    ~FreeOnDestruct();
  private:
    void* ptr;
};

inline FreeOnDestruct::FreeOnDestruct(void* p) :
    ptr(p)
{
}


inline FreeOnDestruct::~FreeOnDestruct() {
    free(ptr);
}


#endif
