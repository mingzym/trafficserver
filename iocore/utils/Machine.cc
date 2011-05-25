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

#include "libts.h"
#include "I_Machine.h"

// Singleton
static Machine *machine = NULL;

// Moved from HttpTransactHeaders.cc, we should probably move this somewhere ...
namespace {
inline char H(int x) {
  x &= 0xF;
  return x>9 ? x - 10 + 'A': x + '0';
}

int
nstrhex(char *d, void const* src, size_t src_len) {
  char const* s = static_cast<char const*>(src);
  for ( char const* limit = s + src_len ; s < limit ; ++s ) {
    *d++ = H(*s >> 4);
    *d++ = H(*s);
  }
  return src_len * 2;
}
} // anon namespace


// Machine class. TODO: This has to deal with IPv6!
Machine *
this_machine()
{
  if (machine == NULL) {
    ink_assert("need to call create_this_machine before accessing" "this_machine()");
  }
  return machine;
}

void
create_this_machine(char *hostname, sockaddr_storage const* ip)
{
  machine = NEW(new Machine(hostname, ip));
}

Machine::Machine(char *ahostname, sockaddr_storage const* aip)
  : hostname(ahostname)
{
  if (!aip || !ink_inet_is_ip(aip)) {
    addrinfo* ai_info = 0;
    addrinfo  ai_hints;
    char localhost[1024];

    if (!ahostname) {
      ink_release_assert(!gethostname(localhost, sizeof(localhost)-1));
      ahostname = localhost;
    }
    hostname = xstrdup(ahostname);

    ink_inet_init(ip);

    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_flags = AI_ADDRCONFIG;
    int z = getaddrinfo(ahostname, 0, &ai_hints, &ai_info);

    if (0 != z) {
      Warning("unable to DNS %s: %d [%s]", ahostname, z, gai_strerror(z));
    } else {
      addrinfo* x = 0; // best candidate (smallest value) so far.
      for ( addrinfo* i = ai_info ; i ; i = i->ai_next ) {
        if (AF_INET == i->ai_family || AF_INET6 == i->ai_family) {
          if (0 == x) x = i;
          else if (1 == ink_inet_cmp(
              ink_inet_ss_cast(x->ai_addr),
              ink_inet_ss_cast(i->ai_addr)
            ))
            x = i;
        }
      }
      if (x) ink_inet_copy(&ip, ink_inet_ss_cast(x->ai_addr));
      else Warning("unable to find IP address for %s", ahostname);
      freeaddrinfo(ai_info);
    }
    //ip = htonl(ip); for the alpha! TODO
  } else {
    char buff[1024];
    ink_inet_copy(&ip, aip);
//    ip = aip;

    int z = getnameinfo(ink_inet_sa_cast(&ip), sizeof ip, buff, sizeof buff, 0, 0, NI_NAMEREQD);

    if (0 != z) {
      Debug("machine_debug", "unable to reverse DNS %s: %d",
        ink_inet_ntop(&ip, buff, sizeof buff),
        z
      );
    } else {
      hostname = xstrdup(buff);
    }
  }

  if (hostname)
    hostname_len = strlen(hostname);
  else
    hostname_len = 0;

  ip_string = static_cast<char *>(xmalloc(INET6_ADDRSTRLEN));
  ink_inet_ntop(&ip, ip_string, INET6_ADDRSTRLEN);
  ip_string_len = strlen(ip_string);

  ip_hex_string = static_cast<char*>(xmalloc(INK_IP6_SIZE * 2 + 1));
  if (ink_inet_is_ip6(ip))
    ip_hex_string_len = nstrhex(ip_hex_string, &ink_inet_ip6_cast(&ip)->sin6_addr, INK_IP6_SIZE);
  else
    ip_hex_string_len = nstrhex(ip_hex_string, &ink_inet_ip4_addr_cast(&ip), sizeof(in_addr_t));
}

Machine::~Machine()
{
  if (hostname)
    xfree(hostname);
  if (ip_string)
    xfree(ip_string);
  if (ip_hex_string)
    xfree(ip_hex_string);
}
