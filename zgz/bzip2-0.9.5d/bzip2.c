
/*-----------------------------------------------------------*/
/*--- A block-sorting, lossless compressor        bzip2.c ---*/
/*-----------------------------------------------------------*/

/*--
  This file is a part of bzip2 and/or libbzip2, a program and
  library for lossless, block-sorting data compression.

  Copyright (C) 1996-1999 Julian R Seward.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. The origin of this software must not be misrepresented; you must 
     not claim that you wrote the original software.  If you use this 
     software in a product, an acknowledgment in the product 
     documentation would be appreciated but is not required.

  3. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  4. The name of the author may not be used to endorse or promote 
     products derived from this software without specific prior written 
     permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Julian Seward, Cambridge, UK.
  jseward@acm.org
  bzip2/libbzip2 version 0.9.5 of 24 May 1999

  This program is based on (at least) the work of:
     Mike Burrows
     David Wheeler
     Peter Fenwick
     Alistair Moffat
     Radford Neal
     Ian H. Witten
     Robert Sedgewick
     Jon L. Bentley

  For more information on these sources, see the manual.
--*/


/*----------------------------------------------------*/
/*--- and now for something much more pleasant :-) ---*/
/*----------------------------------------------------*/

/*---------------------------------------------*/
/*--
  Place a 1 beside your platform, and 0 elsewhere.
--*/

/*--
  Generic 32-bit Unix.
  Also works on 64-bit Unix boxes.
--*/
#define BZ_UNIX      1

/*--
  Win32, as seen by Jacob Navia's excellent
  port of (Chris Fraser & David Hanson)'s excellent
  lcc compiler.
--*/
#define BZ_LCCWIN32  0

#if defined(_WIN32) && !defined(__CYGWIN32__)
#undef BZ_LCCWIN32
#define BZ_LCCWIN32 1
#undef BZ_UNIX
#define BZ_UNIX 0
#endif


/*---------------------------------------------*/
/*--
  Some stuff for all platforms.
--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include "bzlib.h"

#define ERROR_IF_EOF(i)       { if ((i) == EOF)  ioError(); }
#define ERROR_IF_NOT_ZERO(i)  { if ((i) != 0)    ioError(); }
#define ERROR_IF_MINUS_ONE(i) { if ((i) == (-1)) ioError(); }


/*---------------------------------------------*/
/*--
   Platform-specific stuff.
--*/

#if BZ_UNIX
#   include <sys/types.h>
#   include <utime.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <sys/times.h>

#   define PATH_SEP    '/'
#   define MY_LSTAT    lstat
#   define MY_S_IFREG  S_ISREG
#   define MY_STAT     stat

#   define APPEND_FILESPEC(root, name) \
      root=snocString((root), (name))

#   define APPEND_FLAG(root, name) \
      root=snocString((root), (name))

#   define SET_BINARY_MODE(fd) /**/

#   ifdef __GNUC__
#      define NORETURN __attribute__ ((noreturn))
#   else
#      define NORETURN /**/
#   endif
#   ifdef __DJGPP__
#     include <io.h>
#     include <fcntl.h>
#     undef MY_LSTAT
#     define MY_LSTAT stat
#     undef SET_BINARY_MODE
#     define SET_BINARY_MODE(fd)                        \
        do {                                            \
           int retVal = setmode ( fileno ( fd ),        \
                                 O_BINARY );            \
           ERROR_IF_MINUS_ONE ( retVal );               \
        } while ( 0 )
#   endif
#endif



#if BZ_LCCWIN32
#   include <io.h>
#   include <fcntl.h>
#   include <sys\stat.h>

#   define NORETURN       /**/
#   define PATH_SEP       '\\'
#   define MY_LSTAT       _stat
#   define MY_STAT        _stat
#   define MY_S_IFREG(x)  ((x) & _S_IFREG)

#   define APPEND_FLAG(root, name) \
      root=snocString((root), (name))

#   if 0
   /*-- lcc-win32 seems to expand wildcards itself --*/
#   define APPEND_FILESPEC(root, spec)                \
      do {                                            \
         if ((spec)[0] == '-') {                      \
            root = snocString((root), (spec));        \
         } else {                                     \
            struct _finddata_t c_file;                \
            long hFile;                               \
            hFile = _findfirst((spec), &c_file);      \
            if ( hFile == -1L ) {                     \
               root = snocString ((root), (spec));    \
            } else {                                  \
               int anInt = 0;                         \
               while ( anInt == 0 ) {                 \
                  root = snocString((root),           \
                            &c_file.name[0]);         \
                  anInt = _findnext(hFile, &c_file);  \
               }                                      \
            }                                         \
         }                                            \
      } while ( 0 )
#   else
#   define APPEND_FILESPEC(root, name)                \
      root = snocString ((root), (name))
#   endif

#   define SET_BINARY_MODE(fd)                        \
      do {                                            \
         int retVal = setmode ( fileno ( fd ),        \
                               O_BINARY );            \
         ERROR_IF_MINUS_ONE ( retVal );               \
      } while ( 0 )

#endif


/*---------------------------------------------*/
/*--
  Some more stuff for all platforms :-)
--*/

typedef char            Char;
typedef unsigned char   Bool;
typedef unsigned char   UChar;
typedef int             Int32;
typedef unsigned int    UInt32;
typedef short           Int16;
typedef unsigned short  UInt16;
                                       
#define True  ((Bool)1)
#define False ((Bool)0)

/*--
  IntNative is your platform's `native' int size.
  Only here to avoid probs with 64-bit platforms.
--*/
typedef int IntNative;


/*---------------------------------------------------*/
/*--- Misc (file handling) data decls             ---*/
/*---------------------------------------------------*/

Int32   verbosity;
Bool    keepInputFiles, smallMode;
Bool    forceOverwrite, testFailsExist, noisy;
Int32   numFileNames, numFilesProcessed, blockSize100k;


/*-- source modes; F==file, I==stdin, O==stdout --*/
#define SM_I2O           1
#define SM_F2O           2
#define SM_F2F           3

/*-- operation modes --*/
#define OM_Z             1
#define OM_UNZ           2
#define OM_TEST          3

Int32   opMode;
Int32   srcMode;

#define FILE_NAME_LEN 1034

Int32   longestFileName;
Char    inName [FILE_NAME_LEN];
Char    outName[FILE_NAME_LEN];
Char    tmpName[FILE_NAME_LEN];
Char    *progName;
Char    progNameReally[FILE_NAME_LEN];
FILE    *outputHandleJustInCase;
Int32   workFactor;

void    panic                 ( Char* )   NORETURN;
void    ioError               ( void )    NORETURN;
void    outOfMemory           ( void )    NORETURN;
void    blockOverrun          ( void )    NORETURN;
void    badBlockHeader        ( void )    NORETURN;
void    badBGLengths          ( void )    NORETURN;
void    crcError              ( void )    NORETURN;
void    bitStreamEOF          ( void )    NORETURN;
void    cleanUpAndFail        ( Int32 )   NORETURN;
void    compressedStreamEOF   ( void )    NORETURN;

void    copyFileName ( Char*, Char* );
void*   myMalloc ( Int32 );



/*---------------------------------------------------*/
/*--- Processing of complete files and streams    ---*/
/*---------------------------------------------------*/

/*---------------------------------------------*/
Bool myfeof ( FILE* f )
{
   Int32 c = fgetc ( f );
   if (c == EOF) return True;
   ungetc ( c, f );
   return False;
}


/*---------------------------------------------*/
void compressStream ( FILE *stream, FILE *zStream )
{
   BZFILE* bzf = NULL;
   UChar   ibuf[5000];
   Int32   nIbuf;
   UInt32  nbytes_in, nbytes_out;
   Int32   bzerr, bzerr_dummy, ret;

   SET_BINARY_MODE(stream);
   SET_BINARY_MODE(zStream);

   if (ferror(stream)) goto errhandler_io;
   if (ferror(zStream)) goto errhandler_io;

   bzf = bzWriteOpen ( &bzerr, zStream, 
                       blockSize100k, verbosity, workFactor );   
   if (bzerr != BZ_OK) goto errhandler;

   if (verbosity >= 2) fprintf ( stderr, "\n" );

   while (True) {

      if (myfeof(stream)) break;
      nIbuf = fread ( ibuf, sizeof(UChar), 5000, stream );
      if (ferror(stream)) goto errhandler_io;
      if (nIbuf > 0) bzWrite ( &bzerr, bzf, (void*)ibuf, nIbuf );
      if (bzerr != BZ_OK) goto errhandler;

   }

   bzWriteClose ( &bzerr, bzf, 0, &nbytes_in, &nbytes_out );
   if (bzerr != BZ_OK) goto errhandler;

   if (ferror(zStream)) goto errhandler_io;
   ret = fflush ( zStream );
   if (ret == EOF) goto errhandler_io;
   if (zStream != stdout) {
      ret = fclose ( zStream );
      if (ret == EOF) goto errhandler_io;
   }
   if (ferror(stream)) goto errhandler_io;
   ret = fclose ( stream );
   if (ret == EOF) goto errhandler_io;

   if (nbytes_in == 0) nbytes_in = 1;

   return;

   errhandler:
   bzWriteClose ( &bzerr_dummy, bzf, 1, &nbytes_in, &nbytes_out );
   switch (bzerr) {
      case BZ_MEM_ERROR:
         panic ( "out of memory" );
      case BZ_IO_ERROR:
         errhandler_io:
         panic ( "io error" );
      default:
         panic ( "compress:unexpected error" );
   }

   panic ( "compress:end" );
   /*notreached*/
}

void panic (char *msg) {
	perror("oops");
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

int main () {
	/* tunables */
	workFactor              = 30;
	blockSize100k 		= 9;

	compressStream(stdin, stdout);
	exit(0);
}
