/* Copyright (C) 2004 Nadav Har'El and Dan Kenigsberg */

/* The implementation of gzgetc() in the Zlib library, which gets the next
   uncompressed character when reading a gzip file, is extremely slow.
   When we tried using gzgetc() to read the gzipped dictionary file one
   character at a time, the result was 7 times slower start-up than when
   we read from a pipe to the "gzcat" program.

   It turns out that this can easily be solved, by buffering the reads:
   we can read, using gzread() a whole chunk (say, of 4 Kbytes) of
   uncompressed characters, and then dispense them one character at a time,
   much like the stdio library's getc() buffers calls to read().

   This implementation provides a new type, "gzbFile *", routines to open
   and close such a file, gzb_open, gzb_dopen (uses an already open file
   descriptor) and gzb_close, and most importantly, a gzb_getc() routine
   from getting, in a buffered manner, the next uncompressed character
   from the file.

   The semantics implemented is "close enough" to that of zlib to fit
   our needs, but not identical. Also, many other facilities offered
   by zlib and stdio are not given a buffered version here because Hspell
   doesn't need them. Such facilities, like ungetc, scanf, tell/seek,
   and of course writing, can be implemented in the future if needed.
*/

#define GZBUFFERED_SIZE 4096 /* empirical testing showed this to be fine */

#include <stdlib.h>
#include <zlib.h>

typedef struct {
        gzFile gz;
        char buf[GZBUFFERED_SIZE]; /* buffer of preread characters */
        char *b; /* next character to read from b */
        int n; /* number of character left to read in buffer */
} gzbFile;

static inline gzbFile *
gzb_open(const char *path, const char *mode)
{
	gzbFile *ret = (gzbFile *)malloc(sizeof(gzbFile));
	if(!ret)
		return NULL;
	ret->n = 0;
	ret->gz = gzopen(path,mode);
	if(!ret->gz){
		free(ret);
		return NULL;
	}

	return ret;
}

static inline gzbFile *
gzb_dopen(int fd, const char *mode)
{
	gzbFile *ret = (gzbFile *)malloc(sizeof(gzbFile));
	if(!ret)
		return NULL;
	ret->n = 0;
	ret->gz = gzdopen(fd,mode);
	if(!ret->gz){
		free(ret);
		return NULL;
	}
	return ret;
}

static inline int
gzb_close(gzbFile *f)
{
	int ret;
	ret=gzclose(f->gz);
	free(f);
	return ret;
}

static inline int
gzb_getc(gzbFile *gzbp){
	if(!gzbp->n){
		/* No more characters buffered. Refill buffer with gzread() */
		gzbp->n = gzread(gzbp->gz, gzbp->buf, sizeof(gzbp->buf));
		if(gzbp->n <= 0){
			gzbp->n=0;
			return EOF;
		}
		gzbp->b=gzbp->buf;
	}
	/* Return the next available character in the buffer */
	gzbp->n--;
	return *(gzbp->b++);
}

/* We need an implementation of this function for use in linginfo.c
   (which used fgets). This might not be the most efficient implementation -
   we could have browsed the buffer directly, rather than calling gzb_getc
   per character. But I think this will be quick enough.
*/
static inline char *
gzb_gets(char *s, int size, gzbFile *stream)
{
	int c;
	char *ret=s;
	while(--size){ /* stop after at most size-1 characters */
		c=gzb_getc(stream);
		if(c==EOF)
			break;
		else {
			*(s++)=c;
			if(c=='\n')
				break;
		}
	}
	*s='\0';
	return s==ret ? NULL : ret;
}
