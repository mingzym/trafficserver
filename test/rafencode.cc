/************* -*- Mode: C++; Indent: Inktomi4 -*- **************************

  rafencode.cc

  Functions for escapifying and unescapfying for the RAF protocol

  Copied from SF src tree.  Was am-1/misc/rafencode.c v1.10

  Copyright 1999-2000 FastForward Networks, Inc. All Rights Reserved.
  (c) 2001 Inktomi Corporation.  All Rights Reserved.  Confidential.

 ****************************************************************************/

#include <string.h>
#include <stdlib.h>
/*#include "config.h"*/
#include <ctype.h>
#include "rafencode.h"

/*
 * Notes: Maybe create a table for faster encoding.  Not clear that
 * it would be actually faster given the cache pollution issues that
 * a table would potentially cause.  As the CPU outpaces the memory
 * subsystem, reducing the number of accesses to memory become more
 * and more important even if it means a bit more work for the CPU.
 */

/*
 *----------------------------------------------------------------------
 *
 * raf_encodelen --
 *
 *	Compute the size of the buffer that is needed to encode the data
 *	in inbuf using the RAF escaping rules.  Output is not terminated
 *	by an ending '\0'.
 *
 * Arguments:
 *	inbuf:  input buffer
 *	inlen:  number of characters to encode.  If -1, use strlen to compute.
 *	flags:  DISPLAY_ENCODE
 *		If DISPLAY_ENCODE is set, non-printable characters will
 *		be escaped.
 *
 * Results:
 *    Number of characters needed in output buffer for encoding
 *
 *----------------------------------------------------------------------
 */

int
raf_encodelen(const char *inbuf, int inlen, int flags)
{
    const char *p, *end;
    char c;
    int n;
    int spaces = 0;

    if (inlen == -1) {
	inlen = strlen(inbuf);
    }

    p = inbuf;
    end = inbuf + inlen;

    if (flags & RAF_DISPLAY) {
	while (p != end) {
	    if (*p == ' ') {
		spaces = 1;
		break;
	    }
	    p++;
	}
	p = inbuf;
    }

    n = 0;
    /* An empty string will return "" */
    if (inlen == 0) {
	return 2;
    }
    if (spaces) {
	n += 2;
    }
    while (p != end) {
	c = *p;
	if (spaces && c == ' ') {
	    n++;
	}
	else if (c == '\r' || c == '\n' || c == '\t' || c == '\\' || c == ' ' || c == '"') {
	    n += 2;
	} else if (((flags & RAF_DISPLAY) && (!isprint(c)))) {
	    n += 4;
	} else {
	    n++;
	}
	p++;
    }
    return n;
}

/*
 *----------------------------------------------------------------------
 *
 * raf_encode --
 *
 *	Encode the input buffer into the output buffer using the RAF
 *	escaping protocol.
 *
 * Arguments:
 *	inbuf:  input buffer
 *	inlen:  number of characters to encode.  If -1, use strlen to compute.
 *	outbuf: output buffer.  If NULL, function will return the number of
 *		bytes to decode the string.
 *	outlen: number of characters available in output buffer.
 *	flags:  DISPLAY_ENCODE
 *		If DISPLAY_ENCODE is set, non-printable characters will
 *		be escaped.
 * Result:
 *	If outlen was 0, number of bytes needed to decode the string.
 *	Otherwise, the number of bytes copied to the output buffer.
 *
 *----------------------------------------------------------------------
 */

int
raf_encode(const char *inbuf, int inlen, char *outbuf, int outlen, int flags)
{
    const char *ip, *iend;
    char *op, *oend, c;
    int n;
    int spaces = 0;

    if (inlen == -1) {
	inlen = strlen(inbuf);
    }
    if (outbuf == 0) {
	return raf_encodelen(inbuf, inlen, flags);
    }

    ip = inbuf;
    iend = inbuf + inlen;
    if (flags & RAF_DISPLAY) {
	while (ip != iend) {
	    if (*ip == ' ') {
		spaces = 1;
		break;
	    }
	    ip++;
	}
	ip = inbuf;
    }

    op = outbuf;
    oend = outbuf + outlen;
    n = 0;
    /* An empty string will return "" */
    if (inlen == 0) {
	if (op != oend) {
	    *op++ = '"';
	    n++;
	    if (op != oend) {
		*op++ = '"';
		n++;
	    }
	}
    }
    if (op == oend)
	return n;
    if (spaces) {
	*op++ = '"';
	n++;
	if (op == oend)
	    return n;
    }
    while (ip != iend) {
	c = *ip;
	if (c == '\n') {
	    *op++ = '\\';
	    if (op == oend) {
		n++;
		break;
	    }
	    *op++ = 'n';
	    n += 2;
	    if (op == oend) {
		break;
	    }
	} else if (c == '\r') {
	    *op++ = '\\';
	    if (op == oend) {
		n++;
		break;
	    }
	    *op++ = 'r';
	    n += 2;
	    if (op == oend) {
		break;
	    }
	} else if (c == '\t') {
	    *op++ = '\\';
	    if (op == oend) {
		n++;
		break;
	    }
	    *op++ = 't';
	    n += 2;
	    if (op == oend) {
		break;
	    }
	} else if (c == ' ' && spaces) {
	    *op++ = ' ';
	    n++;
	} else if (c == '\\' || c == ' ' || c == '"') {
	    *op++ = '\\';
	    if (op == oend) {
		n++;
		break;
	    }
	    *op++ = c;
	    n += 2;
	    if (op == oend) {
		break;
	    }
	} else if (((flags & RAF_DISPLAY) && (!isprint(c)))) {
	    *op++ = '\\';
	    if (op == oend) {
		n++;
		break;
	    }
	    *op++ = '0' + ((c & 0xc0) >> 6);
	    if (op == oend) {
		n += 2;
		break;
	    }
	    *op++ = '0' + ((c & 0x38) >> 3);
	    if (op == oend) {
		n += 3;
		break;
	    }
	    *op++ = '0' + (c & 0x7);
	    n += 4;
	    if (op == oend) {
		break;
	    }
	} else {
	    *op++ = *ip;
	    n++;
	    if (op == oend) {
		break;
	    }
	}
	ip++;
    }
    if (spaces && op != oend) {
	*op++ = '"';
	n++;
    }
    return n;
}

#define isoctal(c) ((c) >= '0' && (c) <= '7')

/*
 *----------------------------------------------------------------------
 *
 * raf_decodelen --
 *
 *	Compute the number of characters that would be required to
 *	decode the data that was encoded with the RAF encoding
 *      rules.
 *
 * Arguments:
 *	inbuf:  input buffer
 *	inlen:  number of characters to encode.  If -1, use strlen to compute.
 *	lastp:  Returns one past last position that was handled in the string.
 *		This may indicate an argument break or the end of the string.
 *
 *----------------------------------------------------------------------
 */

int
raf_decodelen(const char *inbuf, int inlen, const char **lastp)
{
    const char *p, *end;
    char c;
    int n, inquote;

    if (inlen == -1) {
	inlen = strlen(inbuf);
    }

    p = inbuf;
    end = inbuf + inlen;
    inquote = 0;
    n = 0;
    while (p != end && isspace(*p)) {
	p++;
    }
    while (p != end) {
	c = *p;
	if (c == '"') {
	    inquote = !inquote;
	} else if (c == '\\') {
	    p++;
	    if (p == end) {
		break;
	    }
	    if (end - p >= 3 &&
		isoctal(p[0]) && isoctal(p[1]) && isoctal(p[2])) {
		p += 2;
	    }
	    n++;
	} else if (!inquote && c == ' ') {
	    /* Spaces are not allowed without escaping so it must be the
	     * end of an argument */
	    break;
	} else {
	    n++;
	}
	p++;
    }
    if (lastp) {
	*lastp = p;
    }
    return n;
}


/*
 *----------------------------------------------------------------------
 *
 * raf_decode --
 *
 *	Decode the data in inbuf that is encoded using the RAF escaping
 *	into outbuf.
 *
 * Arguments:
 *	inbuf:  input buffer
 *	inlen:  number of characters to encode.  If -1, use strlen to compute.
 *	outbuf: output buffer.  If NULL, function will return the number of
 *		bytes to decode the string.
 *	outlen: number of characters available in output buffer.
 *	lastp:  Returns one past last position that was handled in the string.
 *		This may indicate an argument break or the end of the string.
 *
 *----------------------------------------------------------------------
 */

int
raf_decode(const char *inbuf, int inlen, char *outbuf, int outlen,
	   const char **lastp)
{
    const char *ip, *iend;
    char *op, *oend, c;
    int inquote;

    if (inlen == -1) {
	inlen = strlen(inbuf);
    }
    if (outbuf == 0) {
	return raf_decodelen(inbuf, inlen, 0);
    }

    ip = inbuf;
    iend = inbuf + inlen;
    op = outbuf;
    oend = outbuf + outlen;
    inquote = 0;
    while (ip != iend && isspace(*ip)) {
	ip++;
    }
    while (ip != iend) {
	c = *ip;
	if (c == '"') {
	    inquote = !inquote;
	    ip++;
	    continue;
	} else if (c == '\\') {
	    ip++;
	    if (ip == iend) {
		break;
	    }
	    c = *ip;
	    if (iend - ip >= 3 &&
		isoctal(ip[0]) && isoctal(ip[1]) && isoctal(ip[2])) {
		if (op == oend) break;
		*op = ((ip[0] - '0') << 6) | ((ip[1] - '0') << 3) | (ip[2] - '0');
		ip += 2;
	    } else if (c == 'n') {
		if (op == oend) break;
		*op = '\n';
	    } else if (c == 'r') {
		if (op == oend) break;
		*op = '\r';
	    } else if (c == 't') {
		if (op == oend) break;
		*op = '\t';
	    } else {
		if (op == oend) break;
		*op = *ip;
	    }
	} else if (!inquote && c == ' ') {
	    /* Spaces are not allowed without escaping so it must be the
	     * end of an argument */
	    break;
	} else {
	    if (op == oend) break;
	    *op = *ip;
	}
	ip++; op++;
    }
    if (lastp) {
	if (inquote && *ip == '"') {
	    ip++;
	}
	*lastp = ip;
    }
    return op - outbuf;
}


/* #define TEST 1 */
#undef TEST
#ifdef TEST

#include <stdlib.h>
#include <stdio.h>

char *encode_tests[] = {
    "",
    "abc\001def\" \n\t",
    "abcdef ghij klm \001\002\003\004\"\"\" \"",
    " ",
    "\\ \\ \\\\\"",
    "Chicago setting tree: { name ISP m1 10000  d 0 m2 10000 excess 0 children { { name IBM m1 2000 d 0 m2 2000 excess 0 children { } } { name ESPN m1 3000 d 0 m2 3000 excess 0 children { } } { name BestEffort m1 500 d 0 m2 500 excess 0 children { } } { name ABC m1 2400 d 0 m2 2400 excess 0 children { } } } }  "
};

char *simple_tests[] = {
    "String\\ 1\\n",
    "\"Quoted String\"",
    "\\001\\000\"Quoted\\ \\001\\\"\"",
    "\\t\\n\"\\ ",
    "\\ \\ \\ \\ \\ \\ \\ ",
};

char *multiarg_tests[] = {
    "  ",
    " \"\"  \"\"",
    "Arg1   Arg2   Arg3",
    "Arg\\ 1 \"Argument 2 is\\ still going\" Arg\\ 3",
    "\"Quoted String with   spaces in it and some \\060\\061\\ raf codes\" arg2 arg3",
    "\"Arg\\\" 1\\\"\" Arg\\ 2 \"\" Arg-4",
};


int
main(int argc, char **argv)
{
    char buf[500], *tmp;
    char *s, *p, *testp, *end;
    int d, i, j, k, n, size, pass, flags;

    /* Visual inspection to see if things look good */
    for (i = 0; i < sizeof(encode_tests)/sizeof(char*); i++) {
	s = encode_tests[i];
	n = raf_encode(s, strlen(s), buf, sizeof(buf), RAF_DISPLAY);
	printf("%s\n=> ", s);
	for (k = 0; k < n; k++) {
	    printf("%c", buf[k]);
	}
	printf("\n");
    }

    /* check for memory corruption by playing some games*/
    for (pass = 0; pass < 2; pass++) {
	printf("----------------------------------------\n");
	flags = (pass == 0) ? 0 : RAF_DISPLAY;
	for (i = 0; i < sizeof(encode_tests)/sizeof(char*); i++) {
	    s = encode_tests[i];
	    size = raf_encodelen(s, strlen(s), flags);
	    tmp = malloc(size+10);

	    printf("In : %s\n", s);
	    for (d = 0; d < size; d++) {
		tmp[size-d] = 'Z';
		n = raf_encode(s, strlen(s), tmp, size-d, flags);
		if (tmp[size-d] != 'Z') {
		    fprintf(stderr, "Error: Memory overwrite\n");
		}
		if (d == 0) {
		    if (n != size) {
			fprintf(stderr,
				"Error: wrong # characters encoded: size(%d) != n(%d)\n",
				size-d, n);
		    }
		    printf("Out: ");
		    for (k = 0; k < n; k++) {
			printf("%c", tmp[k]);
		    }
		    printf("\n");
		}
	    }

	    tmp[size] = 'Z';
	    n = raf_encode(s, strlen(s), tmp, size+10, flags);
	    if (tmp[size] != 'Z') {
		fprintf(stderr, "Error: Memory overwrite\n");
	    }
	    if (n != size) {
		fprintf(stderr,
			"Error: wrong # characters encoded: size(%d) != n(%d)\n",
			size, n);
	    }
	    free(tmp);
	}
    }

    /* Okay, let's see how we do on multiargument lines */
    for (i = 0; i < sizeof(multiarg_tests)/sizeof(char*); i++) {
	printf("----------------------------------------\n");
	s = multiarg_tests[i];
	end = s + strlen(s);
	j = 0;
	while (s != end) {
	    printf("[Argument %d] ", j);
	    size = raf_decodelen(s, strlen(s), &p);

	    tmp = malloc(size + 10);

	    printf("In : %s\n", s);
	    for (d = 0; d <= size; d++) {
		tmp[size-d] = 'Z';
		n = raf_decode(s, strlen(s), tmp, size-d, &testp);
		if (tmp[size-d] != 'Z') {
		    fprintf(stderr, "Error: Memory overwrite\n");
		}
		if (d == 0) {
		    if (p != testp) {
			fprintf(stderr, "End pointer position mismatch: %p != %p\n", p, testp);
		    }
		    if (n != size) {
			fprintf(stderr,
				"Error: wrong # characters encoded: size(%d) != n(%d)\n",
				size-d, n);
		    }
		    printf("Out: ");
		    for (k = 0; k < n; k++) {
			printf("%c", tmp[k]);
		    }
		    printf("\n");
		}
	    }
	    j++;
	    free(tmp);

	    s = p;
	}
    }

    /*
     * Now, give it random data is see if it chokes after doing an
     * encode/decode operation on it.
     */
}
#endif
