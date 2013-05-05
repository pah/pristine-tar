/* zip.c -- compress files to the gzip or pkzip format

   Copyright (C) 1997, 1998, 1999, 2006, 2007 Free Software Foundation, Inc.
   Copyright (C) 1992-1993 Jean-loup Gailly
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

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "gzip.h"

uch outbuf[OUTBUFSIZ];
unsigned outcnt; /* bytes in output buffer */

static ulg crc;  /* crc on uncompressed file data */

static int ifd;  /* input file descriptor */
static int ofd;  /* output file descriptor */

static off_t bytes_in;  /* number of input bytes */

/* ===========================================================================
 * Deflate in to out.
 * IN assertions: the input and output buffers are cleared.
 */
void gnuzip(int in, int out, char *origname, ulg timestamp, int level, int osflag, int rsync, int newrsync)
{
    uch  flags = 0;         /* general purpose bit flags */
    ush  deflate_flags = 0; /* pkzip -es, -en or -ex equivalent */

    ifd = in;
    ofd = out;
    outcnt = 0;
    bytes_in = 0L;

    /* Write the header to the gzip file. */

    put_byte(GZIP_MAGIC[0]); /* magic header */
    put_byte(GZIP_MAGIC[1]);
    put_byte(DEFLATED);      /* compression method */

    if (origname)
	flags |= ORIG_NAME;
    put_byte(flags);         /* general flags */
    put_long(timestamp);

    /* Write deflated file to zip file */
    crc = updcrc(0, 0);

    bi_init(out);
    ct_init();
    lm_init(level, &deflate_flags);

    put_byte((uch)deflate_flags); /* extra flags */
    put_byte(osflag);            /* OS identifier */

    if (origname) {
	char *p = origname;
	do {
	    put_byte(*p);
	} while (*p++);
    }

    gnu_deflate(level, rsync, newrsync);

    /* Write the crc and uncompressed size */
    put_long(crc);
    put_long((ulg)bytes_in);

    flush_outbuf();
}


/* ===========================================================================
 * Read a new buffer from the current input file, perform end-of-line
 * translation, and update the crc and input file size.
 * IN assertion: size >= 2 (for end-of-line translation)
 */
int file_read(char *buf, unsigned size)
{
    unsigned len;

    len = read_buffer (ifd, buf, size);
    if (len == 0) return (int)len;
    if (len == (unsigned)-1) {
	read_error();
	return EOF;
    }

    crc = updcrc((uch*)buf, len);
    bytes_in += (off_t)len;
    return (int)len;
}

/* ===========================================================================
 * Write the output buffer outbuf[0..outcnt-1].
 * (used for the compressed data only)
 */
void flush_outbuf(void)
{
    if (outcnt == 0) return;

    write_buf(ofd, (char *)outbuf, outcnt);
    outcnt = 0;
}
