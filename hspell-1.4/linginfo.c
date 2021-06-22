/* Copyright (C) 2003-2004 Nadav Har'El and Dan Kenigsberg */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linginfo.h"

#include "dmask.c"


/* For an explanation of this bizarre set of definitions, see the comment
   in dict_radix.c, before a similar set. */
#ifdef HAVE_ZLIB
#define BUFFERED_ZLIB
#undef FILE
#undef pclose
#undef pclose
#undef getc
#ifdef BUFFERED_ZLIB
#include "gzbuffered.h"
#undef gzopen
#undef gzdopen
#define FILE void /* void* can be either normal FILE* or gzbFile*. Eek. */
#define popen(path,mode) gzb_open(path,mode)
#define gzopen(path,mode) gzb_open(path,mode)
#define gzdopen(path,mode) gzb_dopen(path,mode)
#define pclose(f) (gzb_close((gzbFile *)(f)))
#define getc(f) (gzb_getc(((gzbFile *)(f))))
#define fgets(s,n,f) (gzb_gets((s),(n),((gzbFile *)(f))))
#else
#include <zlib.h>
#define FILE void    /* FILE* is void*, a.k.a. voidp or gzFile */
#define pclose(f) (gzclose((f)))
#define popen(path,mode) (gzopen((path),(mode)))
#define getc(f) (gzgetc((f)))
#define fgets(s,n,f) (gzgets((s),(n),(f)))
#endif
#undef fgetc
#define fgetc(f) getc(f)
#endif /* HAVE_ZLIB */

static char *flat, **lookup;
static int lookuplen;
extern int hspell_debug;

static int dcode2dmask(const char *dcode) {
	int i = dcode[0]-'A'+(dcode[1]-'A')*26;
	return dmasks[i];
}

static char *dmask2text(char *s, int dmask) {
	char *c;
	s[0]=0;
	switch(dmask & D_TYPEMASK) {
		case D_NOUN: c="ע"; break;
		case D_VERB: c="פ"; break;
		case D_ADJ: c="ת"; break;
	        case 0: c="x"; break;
	        default: c="";
	}
	strcat(s,c);
	/* In few cases, both masculine and faminine are possible */
	if(dmask & D_GENDERMASK & D_MASCULINE) { strcat(s,",ז"); }
	if(dmask & D_GENDERMASK & D_FEMININE) { strcat(s,",נ"); }

	switch(dmask & D_GUFMASK) {
		case D_FIRST: c=",1"; break;
		case D_SECOND: c=",2"; break;
		case D_THIRD: c=",3"; break;
		default: c="";
	}
	strcat(s,c);
	switch(dmask & D_NUMMASK) {
		case D_SINGULAR: c=",יחיד"; break;
		case D_DOUBLE: c=",זוגי"; break;
		case D_PLURAL: c=",רבים"; break;
		default: c="";
	}
	strcat(s,c);
	switch(dmask & D_TENSEMASK) {
		case D_PAST: c=",עבר"; break;
		case D_PRESENT: c=",הווה"; break;
		case D_FUTURE: c=",עתיד"; break;
		case D_IMPERATIVE: c=",ציווי"; break;
		case D_INFINITIVE: c=",מקור"; break;
		case D_BINFINITIVE: c=",מקור,ב"; break;
		default: c="";
	}
	strcat(s,c);
	if (dmask & D_SPECNOUN) {strcat(s,",פרטי");}
	if (dmask & D_OSMICHUT) {strcat(s,",סמיכות");}
	if (dmask & D_OMASK) {
	strcat(s,",כינוי/");
	switch(dmask & D_OGENDERMASK) {
		case D_OMASCULINE: c="ז"; break;
		case D_OFEMININE: c="נ"; break;
		default: c="";
	}
	strcat(s,c);
	switch(dmask & D_OGUFMASK) {
		case D_OFIRST: c=",1"; break;
		case D_OSECOND: c=",2"; break;
		case D_OTHIRD: c=",3"; break;
		default: c="";
	}
	strcat(s,c);
	switch(dmask & D_ONUMMASK) {
		case D_OSINGULAR: c=",יחיד"; break;
		case D_ODOUBLE: c=",זוגי"; break;
		case D_OPLURAL: c=",רבים"; break;
		default: c="";
	}
	strcat(s,c);

	}

	return s;
}

char *linginfo_desc2text(char *text, const char *desc, int i) {
	int dmask;
	if (desc[i*2]==0) return 0;
	dmask = dcode2dmask(&desc[i*2]);
	dmask2text(text,dmask);
	return text;
}

/* find the prefixes required by a word according to its details */
static int linginfo_dmask2ps(int dmask) {
	int specifier;
	if ((dmask&D_TYPEMASK)==D_VERB) {
		if ((dmask&D_TENSEMASK)==D_IMPERATIVE) {
			specifier = PS_IMPER;
		} else if ((dmask&D_TENSEMASK)!=D_PRESENT) {
			specifier = PS_VERB;
		} else if (dmask & D_OSMICHUT || dmask & D_OMASK) {
			specifier = PS_NONDEF;
		} else specifier = PS_ALL;
/* TODO I feel that this may lead to a bug with ליפול and other infinitives that
 * did not loose their initial lamed.  I should correct this all the way from
 * woo.pl */
		if ((dmask&D_TENSEMASK)==D_INFINITIVE) specifier = PS_L;
		else if ((dmask&D_TENSEMASK)==D_BINFINITIVE) specifier = PS_B;
	} else if (((dmask&D_TYPEMASK)==D_NOUN) || ((dmask&D_TYPEMASK) == D_ADJ)) {
		if (dmask & D_OSMICHUT || dmask & D_OMASK
		    || dmask & D_SPECNOUN) {
			specifier = PS_NONDEF;
		} else {
			specifier = PS_ALL;
		}
	} else specifier = PS_ALL;
	return specifier;
}

int linginfo_desc2ps(const char *desc, int i) {
	int dmask;
	if (desc[i*2]==0) return 0;
	dmask = dcode2dmask(&desc[i*2]);
	return linginfo_dmask2ps(dmask);
}

char *linginfo_stem2text(const char *stem, int i) {
	int wp;
	if (stem[i*3]==0) return 0;
   	wp = stem[i*3]-33+(stem[i*3+1]-33)*94+
		(stem[i*3+2]-33)*94*94;
	return lookup[wp];
}

/* currently linginfo_init reopens the words file, reinterprets it, and stores
 * it flat in memory. If it sounds silly to you, you probably can hear. */
int linginfo_init(const char *dir) {
	FILE *fp,*fpstems,*fpdesc;
	char *current;
	char s[1024],stem[100],desc[100];
	int i=0,j;
	int flatsize;

	snprintf(s,sizeof(s),"%s.sizes",dir);
	if(!(fp=fopen(s,"r"))){
		fprintf(stderr,"Hspell: can't open %s.\n",s);
		return 0;
	}
	fscanf(fp,"%*d %*d %*d"); /* ignore non linginfo sizes */
	if(fscanf(fp,"%d %d",&flatsize,&lookuplen)!=2){
		fprintf(stderr,"Hspell: can't read from %s.\n",s);
		return 0;
	}
	fclose(fp);

	current = flat = (char *)malloc(flatsize);
	lookup = (char **)malloc(sizeof(char *)*lookuplen);
	if (!current || !lookup) {
		fprintf (stderr, "Hspell: alloc failed\n");
		return 0;
	}

	/* read dictionary into memory */

	/* TODO: have better quoting for filename, or use zlib directly */
#ifdef HAVE_ZLIB
	snprintf(s,sizeof(s),"%s",dir);
#else
	snprintf(s,sizeof(s),"gzip -dc '%s'",dir);
#endif
	if(!(fp=popen(s,"r"))){
		fprintf(stderr,"Hspell: can't open %s.\n",s);
		return 0;
	}
#ifdef HAVE_ZLIB
	snprintf(s,sizeof(s),"%s.stems",dir);
#else
	snprintf(s,sizeof(s),"gzip -dc '%s.stems'",dir);
#endif
	if(!(fpstems=popen(s,"r"))){
		fprintf(stderr,"Hspell: can't open %s.\n",s);
		pclose(fp);
		return 0;
	}
#ifdef HAVE_ZLIB
	snprintf(s,sizeof(s),"%s.desc",dir);
#else
	snprintf(s,sizeof(s),"gzip -dc '%s.desc'",dir);
#endif
	if(!(fpdesc=popen(s,"r"))){
		fprintf(stderr,"Hspell: can't open %s.\n",s);
		pclose(fp);
		pclose(fpstems);
		return 0;
	}


	/* The following code for reading wunzip'ed word list is copied from
	 * wunzip.c and repeats what was done dict_radix.c's do_read_dict(). It
	 * would be much nicer to read the word list only once. */
	{
	char sbuf[256];
	int slen=0;
	int c,n;
	while(1){
		c=fgetc(fp);
		if((c>='0' && c<='9') || c==EOF){
			/* new word - output old word first */
			sbuf[slen]='\0';
			lookup[i++] = current;
			for(j=0; j<=slen; j++) current++[0]=sbuf[j];
			if (!fgets(stem,sizeof(stem),fpstems)) {
				fprintf(stderr, "Hspell: linginfo: unexpected end of file in stems file\n");
				return 0;
			}
			if (!fgets(desc,sizeof(desc),fpdesc)) {
				fprintf(stderr, "Hspell: linginfo: unexpected end of file in description file\n");
				return 0;
			}
			for (j=0; desc[j]!='\n' && desc[j]!=0; j++) {
				current++[0]=desc[j];
			}
			current++[0]=0;
			for (j=0; stem[j]!='\n' && stem[j]!=0; j++) {
				current++[0]=stem[j];
			}
			current++[0]=0;
			if (c==EOF) break;

			/* and read how much to go back */
			n=0;
			do {
				/* base 10... */
				n*=10;
				n+=(c-'0');
			} while ((c=fgetc(fp))!=EOF && c>='0' && c<='9');
			slen-=n;
			if(slen<0 || slen >= sizeof(sbuf)-1){
				fprintf(stderr,"Hspell: bad backlength %d... giving up.\n", slen);
				return 0;
			}
			/* we got a new letter c - continue the loop */

		}
		/* word letter - add it */
		if(slen>=sizeof(sbuf)-1){
			fprintf(stderr,"Hspell: word too long... giving up.\n");
			return 0;
		}
		sbuf[slen++]=c;
	}
	}

	pclose(fp);
	pclose(fpstems);
	pclose(fpdesc);

	if (hspell_debug) {
		fprintf (stderr, "linginfo: finished reading %d words and stems\n",i);
	}
	return 1;
}

int linginfo_lookup(const char *word, char **desc, char **stem)
{
	int res,i=0,bottom=0,top=lookuplen;
	while (top>=bottom) {
		if (i==(top-bottom)/2 + bottom) {
			return 0;
		}
		i=(top-bottom)/2 + bottom;
		if (hspell_debug) fprintf(stderr,"bot=%d i=%d top=%d) %s\n",bottom,i,top, lookup[i]);
		res = strcmp(lookup[i],word);
		if (res>0) {top=i;}
		else if (res<0) {bottom=i;}
		else {
			int len,desclen;
		        len = strlen(lookup[i]);
			*desc = lookup[i]+len+1;
			desclen = strlen(*desc);
			*stem = *desc+desclen+1;
			return 1;
		}
	}
	return 0;
}

int linginfo_free(void) {
	if (lookup) {
		free(lookup);
		free(flat);
	}
	return 1;
}

