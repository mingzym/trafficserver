/** @file

  Loading @c IpMap from a configuration file.

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

// Copied from IPRange.cc for backwards compatibility.

# include <ts/IpMap.h>

#define TEST(_x)

// #define TEST(_x) _x

#define ERR_STRING_LEN 100

// Returns 0 if successful, 1 if failed
int
read_addr(char *line, sockaddr_storage* ss, int *i, int n)
{
  int k;
  char s[17];
  while ((*i) < n && isspace(line[(*i)]))
    ++(*i);
  if (*i == n) {
    TEST(printf("Socks Configuration (read_an_ip1): Invalid Syntax in line %s\n", line);
      );
    return 1;
  }
  for (k = 0; k < 17 && (isdigit(line[(*i)]) || (line[(*i)] == '.')); ++k, ++(*i)) {
    s[k] = line[(*i)];
  }
  if (k == 17) {
    TEST(printf("Socks Configuration (read_an_ip2): Invalid Syntax in line %s, k %d\n", line, k);
      );
    return 1;
  }
  s[k] = '\0';
  ++k;
  TEST(printf("IP address read %s\n", s));
  if (0 != ink_inet_pton(s, ss)) {
    TEST(printf("Socks Configuration: Illegal IP address read %s, %u\n", s, *ip);
    );
    return 1;
  }
  return 0;
}

char *
Load_IpMap_From_File(IpMap* map, int fd, const char *key_str) {
  char* zret = 0;
  FILE* f = fdopen(dup(fd), "r"); // dup so we don't close the original fd.
  if (f) zret = Load_IpMap_From_File(map, f, key_str);
  else {
    zret = (char *) xmalloc(ERR_STRING_LEN);
    snprintf(zret, ERR_STRING_LEN, "Unable to reopen file descriptor as stream %d:%s", errno, strerror(errno));
  }
  return zret;
}

// Returns 0 if successful, error string otherwise
char *
Load_IpMap_From_File(IpMap* map, FILE* f, const char *key_str)
{
  int i, n, line_no;
  int key_len = strlen(key_str);
  sockaddr_storage ss;
  char line[MAX_LINE_SIZE];

  // First hardcode 127.0.0.1 into the table
  map->mark(INADDR_LOOPBACK, INADDR_LOOPBACK);

  line_no = 0;
  while (fgets(line, MAX_LINE_SIZE, f)) {
    ++line_no;
    n = strlen(line);
    // Find first white space which terminates the line key.
    for ( i = 0 ; i < n && ! isspace(line[i]); ++i )
      ;
    if (i != key_len || 0 != strncmp(line, key_str, key_len))
      continue;
    // Now look for IP address
    while (true) {
      while (i < n && isspace(line[i]))
        ++i;
      if (i == n)
        break;

      if (read_addr(line, &ss, &i, n) == 1) {
        char *error_str = (char *) xmalloc(ERR_STRING_LEN);
        snprintf(error_str, ERR_STRING_LEN, "Incorrect Syntax in Socks Configuration at Line %d", line_no);
        return error_str;
      }

      while (i < n && isspace(line[i]))
        ++i;
      if (i == n || line[i] == ',') {
        // You have read an IP address. Enter it in the table
        this->mark(&ss, &ss);
        if (i == n)
          break;
        else
          ++i;
      } else if (line[i] == '-') {
        sockaddr_storage ss2;
        // What you have just read is the start of the range,
        // Now, read the end of the IP range
        ++i;
        if (read_addr(line, &ss2, &i, n) == 1) {
          char *error_str = (char *) xmalloc(ERR_STRING_LEN);
          snprintf(error_str, ERR_STRING_LEN, "Incorrect Syntax in Socks Configuration at Line %d", line_no);
          return error_str;
        }
        map->mark(&ss, &ss2);
        while (i < n && isspace(line[i]))
          i++;
        if (i == n)
          break;
        if (line[i] != ',') {
          TEST(printf("Socks Configuration (read_table_from_file1):Invalid Syntax in line %s\n", (char *) line);
            );
          char *error_str = (char *) xmalloc(ERR_STRING_LEN);
          snprintf(error_str, ERR_STRING_LEN, "Incorrect Syntax in Socks Configuration at Line %d", line_no);
          return error_str;
        }
        ++i;
      } else {
        TEST(printf("Socks Configuration (read_table_from_file2):Invalid Syntax in line %s\n", (char *) line);
          );
        char *error_str = (char *) xmalloc(ERR_STRING_LEN);
        snprintf(error_str, ERR_STRING_LEN, "Incorrect Syntax in Socks Configuration at Line %d", line_no);
        return error_str;
      }
    }
  }
  TEST(printf("Socks Conf File Read\n");
    );
  return 0;
}
