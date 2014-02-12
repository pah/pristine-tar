/*
 * Authors: Faidon Liambotis <paravoid@debian.org>
 *          Josh Triplett <josh@joshtriplett.org>
 *
 * This is a zlib-based gzip that is heavily based on NetBSD's gzip,
 * developed by Matthew R. Green.
 *
 * This is suited for gzip regeneration and is part of pristine-tar.
 * As such, it adds some extra options which are needed to successfully
 * reproduce the gzips out there and removes features of the original
 * implementation that were not relevant (e.g. decompression)
 *
 * It also has a mode to generate old bzip2 files.
 *
 * Copyright (c) 1997, 1998, 2003, 2004, 2006 Matthew R. Green
 * Copyright (c) 2007 Faidon Liambotis
 * Copyright (c) 2008 Josh Triplett
 * Copyright (c) 2010 Joey Hess
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * gzip.c -- GPL free gzip using zlib.
 *
 * RFC 1950 covers the zlib format
 * RFC 1951 covers the deflate format
 * RFC 1952 covers the gzip format
 *
 */

#define _GNU_SOURCE

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <zlib.h>
#include <libgen.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>

extern void gnuzip(int in, int out, char *origname, unsigned long timestamp, int level, int osflag, int rsync, int newrsync);
extern void old_bzip2(int level);

#define BUFLEN		(64 * 1024)

#define GZIP_MAGIC0	0x1F
#define GZIP_MAGIC1	0x8B

#define HEAD_CRC	0x02
#define EXTRA_FIELD	0x04
#define ORIG_NAME	0x08
#define COMMENT		0x10

#define GZIP_OS_UNIX	3	/* Unix */
#define GZIP_OS_NTFS	11	/* NTFS */

static	const char	gzip_version[] = "zgz 20100613 based on NetBSD gzip 20060927, GNU gzip 1.3.12, and bzip2 0.9.5d";

static	const char	gzip_copyright[] = \
" Authors: Faidon Liambotis <paravoid@debian.org>\n"
"          Josh Triplett <josh@joshtriplett.org>\n"
"\n"
" Copyright (c) 1997, 1998, 2003, 2004, 2006 Matthew R. Green\n"
" Copyright (c) 2007 Faidon Liambotis\n"
" Copyright (c) 2008 Josh Triplett\n"
" * All rights reserved.\n"
" *\n"
" * Redistribution and use in source and binary forms, with or without\n"
" * modification, are permitted provided that the following conditions\n"
" * are met:\n"
" * 1. Redistributions of source code must retain the above copyright\n"
" *    notice, this list of conditions and the following disclaimer.\n"
" * 2. Redistributions in binary form must reproduce the above copyright\n"
" *    notice, this list of conditions and the following disclaimer in the\n"
" *    documentation and/or other materials provided with the distribution.\n"
" * 3. The name of the author may not be used to endorse or promote products\n"
" *    derived from this software without specific prior written permission.\n"
" *\n"
" * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR\n"
" * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES\n"
" * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\n"
" * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,\n"
" * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,\n"
" * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
" * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED\n"
" * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
" * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
" * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
" * SUCH DAMAGE.";

static	int	qflag;			/* quiet mode */

static	void	maybe_err(const char *fmt, ...)
    __attribute__((__format__(__printf__, 1, 2),noreturn));
static	void	maybe_errx(const char *fmt, ...)
    __attribute__((__format__(__printf__, 1, 2),noreturn));
static	void	gz_compress(int, int, const char *, uint32_t, int, int, int, int, int);
static	void	usage(void);
static	void	display_version(void);
static	void	display_license(void);
static	void	shamble(char *, int);
static	void    rebrain(char *, char *, int);

int main(int, char **p);

static const struct option longopts[] = {
	{ "stdout",		no_argument,		0,	'c' },
	{ "to-stdout",		no_argument,		0,	'c' },
	{ "decompress",		no_argument,		0,	'd' },
	{ "uncompress",		no_argument,		0,	'd' },
	{ "force",		no_argument,		0,	'f' },
	{ "help",		no_argument,		0,	'h' },
	{ "no-name",		no_argument,		0,	'n' },
	{ "name",		no_argument,		0,	'N' },
	{ "quiet",		no_argument,		0,	'q' },
	{ "fast",		no_argument,		0,	'1' },
	{ "best",		no_argument,		0,	'9' },
	{ "ascii",		no_argument,		0,	'a' },
	/* new options */
	{ "gnu",                no_argument,            0,      'G' },
	{ "old-bzip2",          no_argument,            0,      'O' },
	{ "suse-bzip2",         no_argument,            0,      'S' },
	{ "suse-pbzip2",        no_argument,            0,      'P' },
	{ "zlib",               no_argument,            0,      'Z' },
	{ "rsyncable",          no_argument,            0,      'R' },
	{ "new-rsyncable",      no_argument,            0,      'r' },
	{ "no-timestamp",	no_argument,		0,	'm' },
	{ "force-timestamp",	no_argument,		0,	'M' },
	{ "timestamp",		required_argument,	0,	'T' },
	{ "osflag",		required_argument,	0,	's' },
	{ "original-name",	required_argument,	0,	'o' },
	{ "filename",		required_argument,	0,	'F' },
	{ "quirk",		required_argument,	0,	'k' },
	/* end */
	{ "version",		no_argument,		0,	'V' },
	{ "license",		no_argument,		0,	'L' },
	{ NULL,			no_argument,		0,	 0  },
};

int
main(int argc, char **argv)
{
	const char *progname = argv[0];
	int gnu = 0;
	int bzold = 0;
	int bzsuse = 0;
	int pbzsuse = 0;
	int quirks = 0;
	char *origname = NULL;
	uint32_t timestamp = 0;
	int memlevel = 8; /* zlib's default */
	int nflag = 0;
	int mflag = 0;
	int fflag = 0;
	int xflag = -1;
	int ntfs_quirk = 0;
	int level = 6;
	int osflag = GZIP_OS_UNIX;
	int rsync = 0;
	int new_rsync = 0;
	int ch;

	if (strcmp(progname, "gunzip") == 0 ||
	    strcmp(progname, "zcat") == 0 ||
	    strcmp(progname, "gzcat") == 0) {
		fprintf(stderr, "%s: decompression is not supported on this version\n", progname);
		usage();
	}

#define OPT_LIST "123456789acdfhF:GLNnMmqRrT:Vo:k:s:ZOSP"

	while ((ch = getopt_long(argc, argv, OPT_LIST, longopts, NULL)) != -1) {
		switch (ch) {
		case 'G':
			gnu = 1;
			break;
		case 'O':
			bzold = 1;
			break;
		case 'S':
			bzsuse = 1;
			break;
		case 'P':
			pbzsuse = 1;
			break;
		case 'Z':
			break;
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			level = ch - '0';
			break;
		case 'c':
			/* Ignored for compatibility; zgz always uses -c */
			break;
		case 'f':
			fflag = 1;
			break;
		case 'N':
			nflag = 0;
			break;
		case 'n':
			nflag = 1;
			/* no break, n implies m */
		case 'm':
			mflag = 1;
			break;
		case 'M':
			mflag = 0;
			break;
		case 'q':
			qflag = 1;
			break;
		case 's':
			osflag = atoi(optarg);
			break;
		case 'F':
		case 'o':
			origname = optarg;
			break;
		case 'k':
			quirks = 1;
			if (strcmp(optarg, "buggy-bsd") == 0) {
				/* certain archives made with older versions of
				 * BSD variants of gzip */

				/* no name or timestamp information */
				nflag = 1;
				mflag = 1;
				/* maximum compression but without indicating so */
				level = 9;
				xflag = 0;
			} else if (strcmp(optarg, "ntfs") == 0) {
				ntfs_quirk = 1;
				/* no name or timestamp information */
				nflag = 1;
				mflag = 1;
				/* osflag is NTFS */
				osflag = GZIP_OS_NTFS;
			} else if (strcmp(optarg, "perl") == 0) {
				/* Perl's Compress::Raw::Zlib */
				memlevel = 9;

				/* no name or timestamp information */
				nflag = 1;
				mflag = 1;
				/* maximum compression but without indicating so */
				level = 9;
				xflag = 0;
			} else {
				fprintf(stderr, "%s: unknown quirk!\n", progname);
				usage();
			}
			break;
		case 'T':
			timestamp = atoi(optarg);
			break;
		case 'R':
			rsync = 1;
			break;
		case 'r':
			new_rsync = 1;
			break;
		case 'd':
			fprintf(stderr, "%s: decompression is not supported on this version\n", progname);
			usage();
			break;
		case 'a':
			fprintf(stderr, "%s: option --ascii ignored on this version\n", progname);
			break;
		case 'V':
			display_version();
			/* NOTREACHED */
		case 'L':
			display_license();
			/* NOT REACHED */
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;
	argc -= optind;

	if (argc != 0) {
		fprintf(stderr, "%s: filenames not supported; use stdin and stdout\n", progname);
		return 1;
	}

	if (fflag == 0 && isatty(STDOUT_FILENO))
		maybe_errx("standard output is a terminal -- ignoring");

	if (nflag)
		origname = NULL;
	if (mflag)
		timestamp = 0;

	if (gnu) {
		if (quirks) {
			fprintf(stderr, "%s: quirks not supported with --gnu\n", progname);
			return 1;
		}
		gnuzip(STDIN_FILENO, STDOUT_FILENO, origname, timestamp, level, osflag, rsync, new_rsync);
	} else if (bzold) {
		if (quirks) {
			fprintf(stderr, "%s: quirks not supported with --old-bzip2\n", progname);
			return 1;
		}
		old_bzip2(level);
	} else if (bzsuse) {
		shamble("suse-bzip2/bzip2", level);
	} else if (pbzsuse) {
		rebrain("suse-bzip2", "pbzip2", level);
	} else {
		if (rsync || new_rsync) {
			fprintf(stderr, "%s: --rsyncable not supported with --zlib\n", progname);
			return 1;
		}

		gz_compress(STDIN_FILENO, STDOUT_FILENO, origname, timestamp, level, memlevel, osflag, xflag, ntfs_quirk);
	}
	return 0;
}

/* maybe print an error */
void
maybe_err(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	exit(1);
}

/* ... without an errno. */
void
maybe_errx(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
	exit(1);
}

/* compress input to output. */
static void
gz_compress(int in, int out, const char *origname, uint32_t mtime, int level, int memlevel, int osflag, int xflag, int ntfs_quirk)
{
	z_stream z;
	char *outbufp, *inbufp;
	off_t in_tot = 0;
	ssize_t in_size;
	int i, error;
	uLong crc;

	outbufp = malloc(BUFLEN);
	inbufp = malloc(BUFLEN);
	if (outbufp == NULL || inbufp == NULL)
		maybe_err("malloc failed");

	memset(&z, 0, sizeof z);
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = 0;

	i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c%c%c%c%c%s", 
		     GZIP_MAGIC0, GZIP_MAGIC1, Z_DEFLATED,
		     origname ? ORIG_NAME : 0,
		     mtime & 0xff,
		     (mtime >> 8) & 0xff,
		     (mtime >> 16) & 0xff,
		     (mtime >> 24) & 0xff,
		     xflag >= 0 ? xflag :
		     level == 1 ? 4 : level == 9 ? 2 : 0,
		     osflag, origname ? origname : "");
	if (i >= BUFLEN)     
		/* this need PATH_MAX > BUFLEN ... */
		maybe_err("snprintf");
	if (origname)
		i++;

	z.next_out = (unsigned char *)outbufp + i;
	z.avail_out = BUFLEN - i;

	error = deflateInit2(&z, level, Z_DEFLATED,
			     (-MAX_WBITS), memlevel, Z_DEFAULT_STRATEGY);
	if (error != Z_OK)
		maybe_err("deflateInit2 failed");

	crc = crc32(0L, Z_NULL, 0);
	for (;;) {
		if (z.avail_out == 0) {
			if (write(out, outbufp, BUFLEN) != BUFLEN)
				maybe_err("write");

			z.next_out = (unsigned char *)outbufp;
			z.avail_out = BUFLEN;
		}

		if (z.avail_in == 0) {
			in_size = read(in, inbufp, BUFLEN);
			if (in_size < 0)
				maybe_err("read");
			if (in_size == 0)
				break;

			crc = crc32(crc, (const Bytef *)inbufp, (unsigned)in_size);
			in_tot += in_size;
			z.next_in = (unsigned char *)inbufp;
			z.avail_in = in_size;
		}

		error = deflate(&z, Z_NO_FLUSH);
		if (error != Z_OK && error != Z_STREAM_END)
			maybe_errx("deflate failed");
	}

	/* clean up */
	for (;;) {
		size_t len;
		ssize_t w;

		error = deflate(&z, Z_FINISH);
		if (error != Z_OK && error != Z_STREAM_END)
			maybe_errx("deflate failed");

		len = (char *)z.next_out - outbufp;

		/* for a really strange reason, that 
		 * particular byte is decremented */
		if (ntfs_quirk)
			outbufp[10]--;

		w = write(out, outbufp, len);
		if (w == -1 || (size_t)w != len)
			maybe_err("write");
		z.next_out = (unsigned char *)outbufp;
		z.avail_out = BUFLEN;

		if (error == Z_STREAM_END)
			break;
	}

	if (deflateEnd(&z) != Z_OK)
		maybe_errx("deflateEnd failed");

	if (ntfs_quirk) {
		/* write NTFS tail magic (?) */
		i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c", 
			0x00, 0x00, 0xff, 0xff, 0x03, 0x00);
		if (i != 6)
			maybe_err("snprintf");
		if (write(out, outbufp, i) != i)
			maybe_err("write");
	}

	/* write CRC32 and input size (ISIZE) at the tail */
	i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c%c%c", 
		 (int)crc & 0xff,
		 (int)(crc >> 8) & 0xff,
		 (int)(crc >> 16) & 0xff,
		 (int)(crc >> 24) & 0xff,
		 (int)in_tot & 0xff,
		 (int)(in_tot >> 8) & 0xff,
		 (int)(in_tot >> 16) & 0xff,
		 (int)(in_tot >> 24) & 0xff);
	if (i != 8)
		maybe_err("snprintf");
	if (write(out, outbufp, i) != i)
		maybe_err("write");

	free(inbufp);
	free(outbufp);
}

/* runs an external, reanimated compressor program */
static	void
shamble(char *zombie, int level)
{
	char *exec_buf;
	char level_buf[3];
	char *argv[3];
	int i, len;

	len = strlen(PKGLIBDIR) + 1 + strlen(zombie) + 1;
	exec_buf = malloc(len);
	snprintf(exec_buf, len, "%s/%s", PKGLIBDIR, zombie);
	snprintf(level_buf, sizeof(level_buf), "-%i", level);

	i = 0;
	argv[i++] = exec_buf;
	argv[i++] = level_buf;
	argv[i++] = NULL;

	execvp(exec_buf, argv);
	perror("Failed to run external program");
	exit(1);
}

/* swaps in a different library and runs a system program */
static	void
rebrain(char *zombie, char *program, int level)
{
	char *path_buf;
	char level_buf[3];
	char *argv[3];
	int i, len;

#if defined(__APPLE__) && defined(__MACH__)
#	define LD_PATH_VAR "DYLD_LIBRARY_PATH" 
#else
#	define LD_PATH_VAR "LD_LIBRARY_PATH"
#endif

	len = strlen(PKGLIBDIR) + 1 + strlen(zombie) + 1;
	path_buf = malloc(len);
	snprintf(path_buf, len, "%s/%s", PKGLIBDIR, zombie);
	/* FIXME - should append, not overwrite */
	setenv(LD_PATH_VAR, path_buf, 1);
	free(path_buf);

	snprintf(level_buf, sizeof(level_buf), "-%i", level);

	i = 0;
	argv[i++] = program;
	argv[i++] = level_buf;
	argv[i++] = NULL;

	execvp(program, argv);
	perror("Failed to run external program");
	exit(1);
}

/* display usage */
static void
usage(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fprintf(stderr,
    "usage: zgz [-" OPT_LIST "] < <file> > <file>\n"
    " -G --gnu                 use GNU gzip implementation\n"
    " -Z --zlib                use zlib's implementation (default)\n"
    " -O --old-bzip2           generate bzip2 (0.9.5d) output\n"
    " -S --suse-bzip2          generate suse bzip2 output\n"
    " -P --suse-pbzip2         generate suse pbzip2 output\n"
    " -1 --fast                fastest (worst) compression\n"
    " -2 .. -8                 set compression level\n"
    " -9 --best                best (slowest) compression\n"
    " -f --force               force writing compressed data to a terminal\n"
    " -N --name                save or restore original file name and time stamp\n"
    " -n --no-name             don't save original file name or time stamp\n"
    " -m --no-timestamp        don't save original time stamp\n"
    " -M --force-timestemp     save the timestamp even if -n was passed\n"
    " -q --quiet               output no warnings\n"
    " -V --version             display program version\n"
    " -h --help                display this help\n"
    " -o NAME\n"
    "    --original-name NAME  use NAME as the original file name\n"
    " -F NAME --filename NAME  same as --original-name\n"
    " -s --osflag              set the OS flag to something different than 03 (Unix)\n"
    " -T --timestamp SECONDS   set the timestamp to the specified number of seconds\n"
    " \ngnu-specific options:\n"
    " -R --rsyncable           make rsync-friendly archive\n"
    " -r --new-rsyncable       make rsync-friendly archive (new version)\n"
    " \nzlib-specific options:\n"
    " -k --quirk QUIRK         enable a format quirk (buggy-bsd, ntfs, perl)\n");
	exit(0);
}

/* display the license information of NetBSD gzip */
static void
display_license(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fprintf(stderr, "%s\n", gzip_copyright);
	exit(0);
}

/* display the version of NetBSD gzip */
static void
display_version(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	exit(0);
}
