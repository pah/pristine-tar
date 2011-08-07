/* gzip.h -- common declarations for all gzip modules

   Copyright (C) 1997, 1998, 1999, 2001, 2006, 2007 Free Software
   Foundation, Inc.
   Copyright (C) 1992-1993 Jean-loup Gailly.
   Copyright (C) 2008 Josh Triplett

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#define BITS 16

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
#  define __attribute__(x)
# endif
#endif

#define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))

/* I don't like nested includes, but the following headers are used
 * too often
 */
#include <stdio.h>
#include <sys/types.h> /* for off_t */
#include <time.h>
#include <string.h>
#define memzero(s, n)     memset ((s), 0, (n))

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* Return codes from gzip */
#define OK      0
#define ERROR   1

/* Compression methods */
#define DEFLATED    8

#define INBUFSIZ  0x8000  /* input buffer size */
#define OUTBUFSIZ  16384  /* output buffer size */
#define DIST_BUFSIZE 0x8000 /* buffer for distances, see trees.c */

extern uch outbuf[];         /* output buffer */

extern unsigned outcnt; /* bytes in output buffer */

#define NO_FILE  (-1)   /* in memory compression */


#define	GZIP_MAGIC     "\037\213" /* Magic header for gzip files, 1F 8B */
#define	OLD_GZIP_MAGIC "\037\236" /* Magic header for gzip 0.5 = freeze 1.x */
#define PKZIP_MAGIC    "\120\113\003\004" /* Magic header for pkzip files */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

/* internal file attribute */
#define UNKNOWN 0xffff
#define BINARY  0
#define ASCII   1

#define WSIZE 0x8000     /* window size--must be a power of two, and */
                         /*  at least 32K for zip's deflate method */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

/* put_byte is used for the compressed output. */
#define put_byte(c) {outbuf[outcnt++]=(uch)(c); if (outcnt==OUTBUFSIZ)\
   flush_outbuf();}

/* Output a 16 bit value, lsb first */
#define put_short(w) \
{ if (outcnt < OUTBUFSIZ-2) { \
    outbuf[outcnt++] = (uch) ((w) & 0xff); \
    outbuf[outcnt++] = (uch) ((ush)(w) >> 8); \
  } else { \
    put_byte((uch)((w) & 0xff)); \
    put_byte((uch)((ush)(w) >> 8)); \
  } \
}

/* Output a 32 bit value to the bit stream, lsb first */
#define put_long(n) { \
    put_short((n) & 0xffff); \
    put_short(((ulg)(n)) >> 16); \
}

/* Diagnostic functions */
#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

	/* in zip.c: */
extern int file_read(char *buf,  unsigned size);

        /* in deflate.c */
void lm_init(int pack_level, ush *flags);
void gnu_deflate(int pack_level, int rsync, int newrsync);

        /* in trees.c */
void ct_init(void);
int  ct_tally(int pack_level, int dist, int lc);
void flush_block(char *buf, ulg stored_len, int pad, int eof);

        /* in bits.c */
void     bi_init(int zipfile);
void     send_bits(int value, int length);
unsigned bi_reverse(unsigned value, int length);
void     bi_windup(void);
void     copy_block(char *buf, unsigned len, int header);
extern   int (*read_buf)(char *buf, unsigned size);

	/* in util.c: */
extern ulg  updcrc(uch *s, unsigned n);
extern void flush_outbuf(void);
extern void write_buf(int fd, void *buf, unsigned cnt);
extern int read_buffer(int fd, void *buf, unsigned int cnt);
extern void gzip_error(char *m);
extern void read_error(void);
extern void write_error(void);
