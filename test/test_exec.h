/****************** -*- Mode: C++; Indent: Inktomi4 -*- *********************

   test_exec.h
   Author          : Mike Chowla

   Description:

   $Id: test_exec.h,v 1.2 2003-06-01 18:38:29 re1 Exp $

   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#ifndef _TEST_EXEC_H_
#define _TEST_EXEC_H_

#include "List.h"
#include "ink_hash_table.h"

class sio_buffer;
class RafCmd;

struct HostRecord {
  public:
    HostRecord(const char* name);
    ~HostRecord();

    int start();
    const char* lookup_package(const char* pkg_name);
    void update_package_entry(const char* pkg_name, const char* new_pkg, int new_pkg_len);
    char* get_id_str();

    char* arch;    
    char* hostname;
    unsigned int ip;
    int port;

    int fd;
    int next_raf_id;
    sio_buffer* read_buffer;

    InkHashTable* package_table;
    LINK(HostRecord, link);
};

struct InstanceRecord {
  public:
    InstanceRecord(const char* name);
    ~InstanceRecord();
    void add_port_binding(const char* name, const char* value);
    const char* get_port_binding(const char* name);

    char* instance_name;
    HostRecord* host_rec;

    InkHashTable* port_bindings;
    LINK(InstanceRecord, link);
};

struct UserDirInfo {
  public:
    UserDirInfo();
    ~UserDirInfo();

    char* username;
    char* shell;
    
    char* hostname;
    char* ip_str;
    
    char* test_stuff_path;
    char* test_stuff_dir;
    char* test_stuff_path_and_dir;

    char* log_dir;
    char* log_file;

    char* tmp_dir;

    char* log_collator_arg;

    char* package_dir;
    int port;

};

const char* send_raf_cmd(int fd, RafCmd* request, int* timeout_ms);
const char* read_raf_resp(int fd, sio_buffer* buf, RafCmd* response, int* timeout_ms);

int do_raf(HostRecord* hrec, RafCmd* request, RafCmd* response);
int do_raf(int fd, RafCmd* request, RafCmd* response);

HostRecord* create_host_rec(const char* hostname);
HostRecord* find_host_rec(const char* hostname);
InstanceRecord* find_instance_rec(const char* name);

char* find_local_package(const char* pkg_name, const char* arch);

void add_def(const char* name, char* value);
int do_single_substitution(const char* sub_name_start,
			   const char* sub_name_end,
			   sio_buffer* output,
			   int output_warnings=1);
int do_substitutions(const char* src, int len, sio_buffer* output, int* errors);
int do_subs_and_replace(char** src, int* errors);

pid_t reap_child(pid_t pid, int* status, int timeout_ms);
pid_t reap_and_kill_child(pid_t child_pid, int* exit_status);

void check_and_process_kill_signal();
void process_kill_signal();

int roll_log_file(const char* test_name);
void notify_viewer_new_test(const char* test_name);
void notify_viewer_log_roll(const char* test_name);
void notify_viewer_done();

extern int post_to_tinderbox;
extern char tinderbox_machine[256];
extern char tinderbox_tree[256];

extern int save_results;
extern char save_results_dir[512];
extern char save_results_url[512];

void TE_Status(const char* format_str, ...);
void TE_Note(const char* format_str, ...);
void TE_Warning(const char* format_str, ...);
void TE_Error(const char* format_str, ...);
void TE_Fatal(const char* format_str, ...);

#endif
