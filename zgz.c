/*
 * Author: Faidon Liambotis <paravoid@debian.org>
 *
 * This is a zlib-based gzip that is heavily based on NetBSD's gzip,
 * developed by Matthew R. Green.
 *
 * This is suited for gzip regeneration and is part of pristine-tar.
 * As such, it adds some extra options which are needed to successfully
 * reproduce the gzips out there and removes features of the original
 * implementation that were not relevant (e.g. decompression)
 *
 * Copyright (c) 1997, 1998, 2003, 2004, 2006 Matthew R. Green
 * Copyright (c) 2007 Faidon Liambotis
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
#include <fts.h>
#include <libgen.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>

/* what type of file are we dealing with */
enum filetype {
	FT_GZIP,
	FT_LAST,
	FT_UNKNOWN
};

#define GZ_SUFFIX	".gz"

#define BUFLEN		(64 * 1024)

#define GZIP_MAGIC0	0x1F
#define GZIP_MAGIC1	0x8B
#define GZIP_OMAGIC1	0x9E

#define GZIP_TIMESTAMP	(off_t)4
#define GZIP_ORIGNAME	(off_t)10

#define HEAD_CRC	0x02
#define EXTRA_FIELD	0x04
#define ORIG_NAME	0x08
#define COMMENT		0x10

#define GZIP_OS_UNIX	3	/* Unix */
#define GZIP_OS_NTFS	11	/* NTFS */

typedef struct {
    const char	*zipped;
    int		ziplen;
    const char	*normal;	/* for unzip - must not be longer than zipped */
} suffixes_t;
static suffixes_t suffixes[] = {
#define	SUFFIX(Z, N) {Z, sizeof Z - 1, N}
	SUFFIX(GZ_SUFFIX,	""),	/* Overwritten by -S .xxx */
	SUFFIX(GZ_SUFFIX,	""),
	SUFFIX(".z",		""),
	SUFFIX("-gz",		""),
	SUFFIX("-z",		""),
	SUFFIX("_z",		""),
	SUFFIX(".taz",		".tar"),
	SUFFIX(".tgz",		".tar"),
	SUFFIX(GZ_SUFFIX,	""),	/* Overwritten by -S "" */
#undef SUFFIX
};
#define NUM_SUFFIXES (sizeof suffixes / sizeof suffixes[0])

static	const char	gzip_version[] = "zgz 20071002 based on NetBSD gzip 20060927";

static	const char	gzip_copyright[] = \
" Author: Faidon Liambotis <paravoid@debian.org>\n"
"\n"
" Copyright (c) 1997, 1998, 2003, 2004, 2006 Matthew R. Green\n"
" Copyright (c) 2007 Faidon Liambotis\n"
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

static	int	cflag;			/* stdout mode */
static	int	numflag = 6;		/* gzip -1..-9 value */

static	int	fflag;			/* force mode */
static	int	nflag;			/* don't save name (impiles -m) */
static	int	Nflag;			/* restore name/timestamp */
static	int	mflag;			/* undocumented: don't save timestamp */
static	int	qflag;			/* quiet mode */
static	int	tflag;			/* test */
static	int	vflag;			/* verbose mode */
static	int	xflag = -1;		/* don't set Extra Flags (i.e. compression level)
					   binary compatibility with an older, buggy version */
static	int	osflag = GZIP_OS_UNIX;	/* Unix or otherwise */
static	int	ntfs_quirk = 0;		/* whether NTFS quirk is activated */
static	int	exit_value = 0;		/* exit value */

static	char	*infile;		/* name of file coming in */

static	void	maybe_err(const char *fmt, ...)
    __attribute__((__format__(__printf__, 1, 2)));
static	void	maybe_warn(const char *fmt, ...)
    __attribute__((__format__(__printf__, 1, 2)));
static	void	maybe_warnx(const char *fmt, ...)
    __attribute__((__format__(__printf__, 1, 2)));
#if XXX_INPUT
static	enum filetype file_gettype(u_char *);
#endif
static	off_t	gz_compress(int, int, off_t *, const char *, uint32_t);
static	off_t	file_compress(char *, char *, char *, size_t);
static	void	handle_pathname(char *, char *);
static	void	handle_file(char *, char *, struct stat *);
static	void	handle_stdout(char *);
static	void	print_ratio(off_t, off_t, FILE *);
static	void	usage(void);
static	void	display_version(void);
static	void	display_license(void);
static	const suffixes_t *check_suffix(char *, int);

static	void	print_verbage(const char *, const char *, off_t, off_t);
static	void	copymodes(int fd, const struct stat *, const char *file);
static	int	check_outfile(const char *outfile);

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
	{ "recursive",		no_argument,		0,	'r' },
	{ "suffix",		required_argument,	0,	'S' },
	{ "test",		no_argument,		0,	't' },
	{ "verbose",		no_argument,		0,	'v' },
	{ "fast",		no_argument,		0,	'1' },
	{ "best",		no_argument,		0,	'9' },
	{ "ascii",		no_argument,		0,	'a' },
	/* new options */
	{ "no-timestamp",	no_argument,		0,	'm' },
	{ "force-timestamp",	no_argument,		0,	'M' },
	{ "osflag",		required_argument,	0,	's' },
	{ "original-name",	required_argument,	0,	'o' },
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
	char origname[BUFLEN] = { 0 };
	int len;
	int ch;

	if (strcmp(progname, "gunzip") == 0 ||
	    strcmp(progname, "zcat") == 0 ||
	    strcmp(progname, "gzcat") == 0) {
		fprintf(stderr, "%s: decompression is not supported on this version\n", progname);
		usage();
	}

	if (argc > 1 && strcmp(argv[1], "--gnu") == 0) {
		/* omit first argument, i.e. --gnu */
		argv++;
		/* works because "--gnu" is bigger than "gzip" */
		strcpy(argv[0], "gzip"); 
		execv("/bin/gzip", argv);

		/* NOT REACHED */
		fprintf(stderr, "Failed to spawn /bin/gzip\n");
		exit(1);
	} else if (argc > 1 && strcmp(argv[1], "--zlib") == 0) {
		/* skip --zlib argument if existent */
		argc--;
		argv++;
	}

#define OPT_LIST "123456789acdfhLNnMmqrS:tVvo:k:s:"

	while ((ch = getopt_long(argc, argv, OPT_LIST, longopts, NULL)) != -1) {
		switch (ch) {
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			numflag = ch - '0';
			break;
		case 'c':
			cflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'N':
			nflag = 0;
			Nflag = 1;
			break;
		case 'n':
			nflag = 1;
			Nflag = 0;
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
		case 'S':
			len = strlen(optarg);
			if (len != 0) {
				suffixes[0].zipped = optarg;
				suffixes[0].ziplen = len;
			} else {
				suffixes[NUM_SUFFIXES - 1].zipped = "";
				suffixes[NUM_SUFFIXES - 1].ziplen = 0;
			}
			break;
		case 't':
			cflag = 1;
			tflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 's':
			osflag = atoi(optarg);
			break;
		case 'o':
			if (nflag)
				fprintf(stderr, "%s: ignoring original-name because no-name was passed\n", progname);
			strncpy(origname, optarg, BUFLEN);
			break;
		case 'k':
			if (strcmp(optarg, "buggy-bsd") == 0) {
				/* certain archives made with older versions of
				 * BSD variants of gzip */

				/* no name or timestamp information */
				nflag = 1;
				mflag = 1;
				/* maximum compression but without indicating so */
				numflag = 9;
				xflag = 0;
			} else if (strcmp(optarg, "ntfs") == 0) {
				ntfs_quirk = 1;
				/* no name or timestamp information */
				nflag = 1;
				mflag = 1;
				/* osflag is NTFS */
				osflag = GZIP_OS_NTFS;
			} else {
				fprintf(stderr, "%s: unknown quirk!\n", progname);
				usage();
			}
			break;
		case 'r':
			fprintf(stderr, "%s: recursive is not supported on this version\n", progname);
			usage();
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

	if (argc == 0) {
		handle_stdout(origname);
	} else {
		do {
			handle_pathname(argv[0], origname);
		} while (*++argv);
	}
	exit(exit_value);
}

/* maybe print a warning */
void
maybe_warn(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	if (exit_value == 0)
		exit_value = 1;
}

/* ... without an errno. */
void
maybe_warnx(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
	if (exit_value == 0)
		exit_value = 1;
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
	exit(2);
}

/* compress input to output. Return bytes read, -1 on error */
static off_t
gz_compress(int in, int out, off_t *gsizep, const char *origname, uint32_t mtime)
{
	z_stream z;
	char *outbufp, *inbufp;
	off_t in_tot = 0, out_tot = 0;
	ssize_t in_size;
	int i, error;
	uLong crc;

	outbufp = malloc(BUFLEN);
	inbufp = malloc(BUFLEN);
	if (outbufp == NULL || inbufp == NULL) {
		maybe_err("malloc failed");
		goto out;
	}

	memset(&z, 0, sizeof z);
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = 0;

	if (mflag != 0)
		mtime = 0;
	if (nflag != 0)
		origname = "";

	i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c%c%c%c%c%s", 
		     GZIP_MAGIC0, GZIP_MAGIC1, Z_DEFLATED,
		     *origname ? ORIG_NAME : 0,
		     mtime & 0xff,
		     (mtime >> 8) & 0xff,
		     (mtime >> 16) & 0xff,
		     (mtime >> 24) & 0xff,
		     xflag >= 0 ? xflag :
		     numflag == 1 ? 4 : numflag == 9 ? 2 : 0,
		     osflag, origname);
	if (i >= BUFLEN)     
		/* this need PATH_MAX > BUFLEN ... */
		maybe_err("snprintf");
	if (*origname)
		i++;

	z.next_out = (unsigned char *)outbufp + i;
	z.avail_out = BUFLEN - i;

	error = deflateInit2(&z, numflag, Z_DEFLATED,
			     (-MAX_WBITS), 8, Z_DEFAULT_STRATEGY);
	if (error != Z_OK) {
		maybe_warnx("deflateInit2 failed");
		in_tot = -1;
		goto out;
	}

	crc = crc32(0L, Z_NULL, 0);
	for (;;) {
		if (z.avail_out == 0) {
			if (write(out, outbufp, BUFLEN) != BUFLEN) {
				maybe_warn("write");
				in_tot = -1;
				goto out;
			}

			out_tot += BUFLEN;
			z.next_out = (unsigned char *)outbufp;
			z.avail_out = BUFLEN;
		}

		if (z.avail_in == 0) {
			in_size = read(in, inbufp, BUFLEN);
			if (in_size < 0) {
				maybe_warn("read");
				in_tot = -1;
				goto out;
			}
			if (in_size == 0)
				break;

			crc = crc32(crc, (const Bytef *)inbufp, (unsigned)in_size);
			in_tot += in_size;
			z.next_in = (unsigned char *)inbufp;
			z.avail_in = in_size;
		}

		error = deflate(&z, Z_NO_FLUSH);
		if (error != Z_OK && error != Z_STREAM_END) {
			maybe_warnx("deflate failed");
			in_tot = -1;
			goto out;
		}
	}

	/* clean up */
	for (;;) {
		size_t len;
		ssize_t w;

		error = deflate(&z, Z_FINISH);
		if (error != Z_OK && error != Z_STREAM_END) {
			maybe_warnx("deflate failed");
			in_tot = -1;
			goto out;
		}

		len = (char *)z.next_out - outbufp;

		/* for a really strange reason, that 
		 * particular byte is decremented */
		if (ntfs_quirk)
			outbufp[10]--;

		w = write(out, outbufp, len);
		if (w == -1 || (size_t)w != len) {
			maybe_warn("write");
			out_tot = -1;
			goto out;
		}
		out_tot += len;
		z.next_out = (unsigned char *)outbufp;
		z.avail_out = BUFLEN;

		if (error == Z_STREAM_END)
			break;
	}

	if (deflateEnd(&z) != Z_OK) {
		maybe_warnx("deflateEnd failed");
		in_tot = -1;
		goto out;
	}

	if (ntfs_quirk) {
		/* write NTFS tail magic (?) */
		i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c", 
			0x00, 0x00, 0xff, 0xff, 0x03, 0x00);
		if (i != 6)
			maybe_err("snprintf");
		if (write(out, outbufp, i) != i) {
			maybe_warn("write");
			in_tot = -1;
		} else
			out_tot += i;
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
	if (write(out, outbufp, i) != i) {
		maybe_warn("write");
		in_tot = -1;
	} else
		out_tot += i;

out:
	if (inbufp != NULL)
		free(inbufp);
	if (outbufp != NULL)
		free(outbufp);
	if (gsizep)
		*gsizep = out_tot;
	return in_tot;
}


/*
 * set the owner, mode, flags & utimes using the given file descriptor.
 * file is only used in possible warning messages.
 */
static void
copymodes(int fd, const struct stat *sbp, const char *file)
{
	struct stat sb;

	/*
	 * If we have no info on the input, give this file some
	 * default values and return..
	 */
	if (sbp == NULL) {
		mode_t mask = umask(022);

		(void)fchmod(fd, DEFFILEMODE & ~mask);
		(void)umask(mask);
		return; 
	}
	sb = *sbp;

	/* if the chown fails, remove set-id bits as-per compress(1) */
	if (fchown(fd, sb.st_uid, sb.st_gid) < 0) {
		if (errno != EPERM)
			maybe_warn("couldn't fchown: %s", file);
		sb.st_mode &= ~(S_ISUID|S_ISGID);
	}

	/* we only allow set-id and the 9 normal permission bits */
	sb.st_mode &= S_ISUID | S_ISGID | S_IRWXU | S_IRWXG | S_IRWXO;
	if (fchmod(fd, sb.st_mode) < 0)
		maybe_warn("couldn't fchmod: %s", file);
}

#if INPUT
/* what sort of file is this? */
static enum filetype
file_gettype(u_char *buf)
{

	if (buf[0] == GZIP_MAGIC0 &&
	    (buf[1] == GZIP_MAGIC1 || buf[1] == GZIP_OMAGIC1))
		return FT_GZIP;
	else
		return FT_UNKNOWN;
}
#endif

/* check the outfile is OK. */
static int
check_outfile(const char *outfile)
{
	struct stat sb;
	int ok = 1;

	if (stat(outfile, &sb) == 0) {
		if (fflag)
			unlink(outfile);
		else if (isatty(STDIN_FILENO)) {
			char ans[10] = { 'n', '\0' };	/* default */

			fprintf(stderr, "%s already exists -- do you wish to "
					"overwrite (y or n)? " , outfile);
			(void)fgets(ans, sizeof(ans) - 1, stdin);
			if (ans[0] != 'y' && ans[0] != 'Y') {
				fprintf(stderr, "\tnot overwritting\n");
				ok = 0;
			} else
				unlink(outfile);
		} else {
			maybe_warnx("%s already exists -- skipping", outfile);
			ok = 0;
		}
	}
	return ok;
}

static void
unlink_input(const char *file, const struct stat *sb)
{
	struct stat nsb;

	if (stat(file, &nsb) != 0)
		/* Must be gone alrady */
		return;
	if (nsb.st_dev != sb->st_dev || nsb.st_ino != sb->st_ino)
		/* Definitely a different file */
		return;
	unlink(file);
}

static const suffixes_t *
check_suffix(char *file, int xlate)
{
	const suffixes_t *s;
	int len = strlen(file);
	char *sp;

	for (s = suffixes; s != suffixes + NUM_SUFFIXES; s++) {
		/* if it doesn't fit in "a.suf", don't bother */
		if (s->ziplen >= len)
			continue;
		sp = file + len - s->ziplen;
		if (strcmp(s->zipped, sp) != 0)
			continue;
		if (xlate)
			strcpy(sp, s->normal);
		return s;
	}
	return NULL;
}

/*
 * compress the given file: create a corresponding .gz file and remove the
 * original.
 */
static off_t
file_compress(char *file, char *origname, char *outfile, size_t outsize)
{
	int in;
	int out;
	off_t size, insize;
	struct stat isb, osb;
	const suffixes_t *suff;

	in = open(file, O_RDONLY);
	if (in == -1) {
		maybe_warn("can't open %s", file);
		return -1;
	}

	if (cflag == 0) {
		if (fstat(in, &isb) == 0) {
			if (isb.st_nlink > 1 && fflag == 0) {
				maybe_warnx("%s has %lu other link%s -- "
				            "skipping", file,
					    (unsigned long)(isb.st_nlink - 1),
					    isb.st_nlink == 1 ? "" : "s");
				close(in);
				return -1;
			}
		}

		if (fflag == 0 && (suff = check_suffix(file, 0))
		    && suff->zipped[0] != 0) {
			maybe_warnx("%s already has %s suffix -- unchanged",
				    file, suff->zipped);
			close(in);
			return -1;
		}

		/* Add (usually) .gz to filename */
		if ((size_t)snprintf(outfile, outsize, "%s%s",
					file, suffixes[0].zipped) >= outsize)
			memcpy(outfile - suffixes[0].ziplen - 1,
				suffixes[0].zipped, suffixes[0].ziplen + 1);

		if (check_outfile(outfile) == 0) {
			close(in);
			return -1;
		}

		out = open(outfile, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (out == -1) {
			maybe_warn("could not create output: %s", outfile);
			fclose(stdin);
			return -1;
		}
	} else {
		out = STDOUT_FILENO;
	}

	insize = gz_compress(in, out, &size, origname, (uint32_t)isb.st_mtime);

	(void)close(in);

	/*
	 * If there was an error, insize will be -1.
	 * If we compressed to stdout, just return the size.
	 * Otherwise stat the file and check it is the correct size.
	 * We only blow away the file if we can stat the output and it
	 * has the expected size.
	 */
	if (cflag != 0)
		return insize == -1 ? -1 : size;

	if (fstat(out, &osb) != 0) {
		maybe_warn("couldn't stat: %s", outfile);
		goto bad_outfile;
	}

	if (osb.st_size != size) {
		maybe_warnx("output file: %s wrong size (%" PRId64
				" != %" PRId64 "), deleting",
				outfile, (int64_t)osb.st_size,
				(int64_t)size);
		goto bad_outfile;
	}

	copymodes(out, &isb, outfile);
	if (close(out) == -1)
		maybe_warn("couldn't close output");

	/* output is good, ok to delete input */
	unlink_input(file, &isb);
	return size;

    bad_outfile:
	if (close(out) == -1)
		maybe_warn("couldn't close output");

	maybe_warnx("leaving original %s", file);
	unlink(outfile);
	return size;
}

static void
handle_stdout(char *origname)
{
	off_t gsize, usize;
	struct stat sb;
	time_t systime;
	uint32_t mtime;
	int ret;

	if (fflag == 0 && isatty(STDOUT_FILENO)) {
		maybe_warnx("standard output is a terminal -- ignoring");
		return;
	}
	/* If stdin is a file use it's mtime, otherwise use current time */
	ret = fstat(STDIN_FILENO, &sb);

	if (ret < 0) {
		maybe_warn("Can't stat stdin");
		return;
	}

	if (S_ISREG(sb.st_mode))
		mtime = (uint32_t)sb.st_mtime;
	else {
		systime = time(NULL);
		if (systime == -1) {
			maybe_warn("time");
			return;
		} 
		mtime = (uint32_t)systime;
	}
	 		
	usize = gz_compress(STDIN_FILENO, STDOUT_FILENO, &gsize, origname, mtime);
        if (vflag && !tflag && usize != -1 && gsize != -1)
		print_verbage(NULL, NULL, usize, gsize);
}

/* do what is asked for, for the path name */
static void
handle_pathname(char *path, char *origname)
{
	char *opath = path;
	struct stat sb;

	/* check for stdout */
	if (path[0] == '-' && path[1] == '\0') {
		handle_stdout(origname);
		return;
	}

	if (stat(path, &sb) != 0) {
		maybe_warn("can't stat: %s", opath);
		return;
	}

	if (S_ISDIR(sb.st_mode)) {
		maybe_warnx("%s is a directory", path);
		return;
	}

	if (S_ISREG(sb.st_mode))
		handle_file(path, strlen(origname) ? origname : basename(path), &sb);
	else
		maybe_warnx("%s is not a regular file", path);
}

/* compress/decompress a file */
static void
handle_file(char *file, char *origname, struct stat *sbp)
{
	off_t usize, gsize;
	char	outfile[PATH_MAX];

	infile = file;
	gsize = file_compress(file, origname, outfile, sizeof(outfile));
	if (gsize == -1)
		return;
	usize = sbp->st_size;

	if (vflag && !tflag)
		print_verbage(file, (cflag) ? NULL : outfile, usize, gsize);
}

/* print a ratio - size reduction as a fraction of uncompressed size */
static void
print_ratio(off_t in, off_t out, FILE *where)
{
	int percent10;	/* 10 * percent */
	off_t diff;
	char buff[8];
	int len;

	diff = in - out/2;
	if (diff <= 0)
		/*
		 * Output is more than double size of input! print -99.9%
		 * Quite possibly we've failed to get the original size.
		 */
		percent10 = -999;
	else {
		/*
		 * We only need 12 bits of result from the final division,
		 * so reduce the values until a 32bit division will suffice.
		 */
		while (in > 0x100000) {
			diff >>= 1;
			in >>= 1;
		}
		if (in != 0)
			percent10 = ((u_int)diff * 2000) / (u_int)in - 1000;
		else
			percent10 = 0;
	}

	len = snprintf(buff, sizeof buff, "%2.2d.", percent10);
	/* Move the '.' to before the last digit */
	buff[len - 1] = buff[len - 2];
	buff[len - 2] = '.';
	fprintf(where, "%5s%%", buff);
}

/* print compression statistics, and the new name (if there is one!) */
static void
print_verbage(const char *file, const char *nfile, off_t usize, off_t gsize)
{
	if (file)
		fprintf(stderr, "%s:%s  ", file,
		    strlen(file) < 7 ? "\t\t" : "\t");
	print_ratio(usize, gsize, stderr);
	if (nfile)
		fprintf(stderr, " -- replaced with %s", nfile);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/* display the usage of NetBSD gzip */
static void
usage(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fprintf(stderr,
    "usage: %s [--gnu | --zlib] [-" OPT_LIST "] [<file> [<file> ...]]\n"
    " --gnu                    use GNU gzip (/bin/gzip)\n"
    " --zlib                   use zlib's implementation (default)\n"
    " -1 --fast                fastest (worst) compression\n"
    " -2 .. -8                 set compression level\n"
    " -9 --best                best (slowest) compression\n"
    " -c --stdout              write to stdout, keep original files\n"
    "    --to-stdout\n"
    " -f --force               force overwriting & compress links\n"
    " -N --name                save or restore original file name and time stamp\n"
    " -n --no-name             don't save original file name or time stamp\n"
    " -m --no-timestamp        don't save original time stamp\n"
    " -M --force-timestemp     save the timestamp even if -n was passed\n"
    " -S .suf                  use suffix .suf instead of .gz\n"
    "    --suffix .suf\n"
    " -t --test                test compressed file\n"
    " -q --quiet               output no warnings\n"
    " -V --version             display program version\n"
    " -v --verbose             print extra statistics\n"
    " -h --help                display this help\n"
    " \nzlib-specific options:\n"
    " -o NAME\n"
    "    --original-name NAME  use NAME as the original file name\n"
    " -k --quirk QUIRK         enable a format quirk (buggy-bsd, ntfs)\n"
    " -s --osflag              set the OS flag to something different than 03 (Unix)\n",
	    "gzip");
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
