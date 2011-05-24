/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

/**************************************************************************
  Connections

  Commonality across all platforms -- move out as required.

**************************************************************************/

#include "ink_unused.h" /* MAGIC_EDITING_TAG */
#include "P_DNS.h"
#include "P_DNSConnection.h"
#include "P_DNSProcessor.h"

#define SET_TCP_NO_DELAY
#define SET_NO_LINGER
// set in the OS
// #define RECV_BUF_SIZE            (1024*64)
// #define SEND_BUF_SIZE            (1024*64)
#define FIRST_RANDOM_PORT        (16000)
#define LAST_RANDOM_PORT         (60000)

#define ROUNDUP(x, y) ((((x)+((y)-1))/(y))*(y))

//
// Functions
//

DNSConnection::DNSConnection():
  fd(NO_FD), num(0), generator((uint32_t)((uintptr_t)time(NULL) ^ (uintptr_t) this)), handler(NULL)
{
  memset(&sa, 0, sizeof(sockaddr_storage));
}

DNSConnection::~DNSConnection()
{
  close();
}

int
DNSConnection::close()
{
  // don't close any of the standards
  if (fd >= 2) {
    int fd_save = fd;
    fd = NO_FD;
    return socketManager.close(fd_save);
  } else {
    fd = NO_FD;
    return -EBADF;
  }
}

void
DNSConnection::trigger()
{
  handler->triggered.enqueue(this);
}

int
DNSConnection::connect(sockaddr_storage const* target,
                       bool non_blocking_connect, bool use_tcp, bool non_blocking, bool bind_random_port)
{
  ink_assert(fd == NO_FD);

  int res = 0;
  short Proto;
  uint8_t family = target->ss_family;

  if (use_tcp) {
    Proto = IPPROTO_TCP;
    if ((res = socketManager.socket(family, SOCK_STREAM, 0)) < 0)
      goto Lerror;
  } else {
    Proto = IPPROTO_UDP;
    if ((res = socketManager.socket(family, SOCK_DGRAM, 0)) < 0)
      goto Lerror;
  }

  fd = res;

  if (bind_random_port) {
    int retries = 0;
    while (retries++ < 10000) {
      sockaddr_storage bind_sa;
      ink_inet_init(bind_sa);
      if (ink_inet_is_ip6(target))
        ink_inet_ip6_addr_cast(&bind_sa) = in6addr_any;
      else
        ink_inet_ip4_set(&bind_sa, INADDR_ANY);
      uint32_t p = generator.random();
      p = static_cast<uint16_t>((p % (LAST_RANDOM_PORT - FIRST_RANDOM_PORT)) + FIRST_RANDOM_PORT);
      ink_inet_port_cast(bind_sa) = htons(p);
      Debug("dns", "random port = %u\n", p);
      if ((res = socketManager.ink_bind(fd, &bind_sa, sizeof(bind_sa), Proto)) < 0) {
        continue;
      }
      goto Lok;
    }
    Warning("unable to bind random DNS port");
  Lok:;
  }

  if (non_blocking_connect)
    if ((res = safe_nonblocking(fd)) < 0)
      goto Lerror;

  // cannot do this after connection on non-blocking connect
#ifdef SET_TCP_NO_DELAY
  if (use_tcp)
    if ((res = safe_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, ON, sizeof(int))) < 0)
      goto Lerror;
#endif
#ifdef RECV_BUF_SIZE
  socketManager.set_rcvbuf_size(fd, RECV_BUF_SIZE);
#endif
#ifdef SET_SO_KEEPALIVE
  // enables 2 hour inactivity probes, also may fix IRIX FIN_WAIT_2 leak
  if ((res = safe_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, ON, sizeof(int))) < 0)
    goto Lerror;
#endif

  res =::connect(fd, ink_inet_sa_cast(target), sizeof(*target));

  if (!res || ((res < 0) && (errno == EINPROGRESS || errno == EWOULDBLOCK))) {
    if (!non_blocking_connect && non_blocking)
      if ((res = safe_nonblocking(fd)) < 0)
        goto Lerror;
  } else
    goto Lerror;

  return 0;

Lerror:
  if (fd != NO_FD)
    close();
  return res;
}
