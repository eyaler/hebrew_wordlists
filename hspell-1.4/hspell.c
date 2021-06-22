/* Copyright (C) 2003-2017 Nadav Har'El and Dan Kenigsberg */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "hash.h"
#include "hspell.h"
#ifdef USE_LINGINFO
#include "linginfo.h"
#endif

/* load_personal_dict tries to load ~/.hspell_words and ./hspell_words.
   Currently, they are read into a hash table, where each word in the
   file gets a non-zero value.
   Empty lines starting with # are ignored. Lines containing non-Hebrew
   characters aren't ignored, but they won't be tried as questioned words
   anyway.

   If a non-null int pointer is given as a second parameter, the pointed
   value is set to 1 if a personal dictionary was found in the current
   directory, or to 0 otherwise (it was found in the user's home directory,
   or none was found). This knowledge is useful when a modified personal
   wordlist is to be saved, and we want to know if to save it in the
   current directory, or home directory.
*/
static void
load_personal_dict(hspell_hash *personaldict, int *currentdir_dictfile)
{
	int i;
	hspell_hash_init(personaldict);
	if (currentdir_dictfile)
		*currentdir_dictfile = 0;
	for(i=0; i<=1; i++){
		char buf[512];
		FILE *fp;
		if(i==0){
			char *home = getenv("HOME");
			if(!home) continue;
			snprintf(buf, sizeof(buf),
				 "%s/.hspell_words", home);
		} else
			snprintf(buf, sizeof(buf), "./hspell_words");
		fp=fopen(buf, "r");
		if(!fp) continue;
		if (i == 1 && currentdir_dictfile)
			*currentdir_dictfile = 1;
		while(fgets(buf, sizeof(buf), fp)){
			int l=strlen(buf);
			if(buf[l-1]=='\n')
				buf[l-1]='\0';
			if(buf[0]!='#' && buf[0]!='\0')
				hspell_hash_incr_int(personaldict, buf);
		}
		fclose(fp);
	}
}

/* save_personal_dict() saves the personal dictionary to disk. It does this
   by appending the words in personaldict_new_words to the dictionary file
   (the one in the current directory, if that had been read, otherwise
   the one in the home directory)..
   Returns non-zero on success.
*/
static int
save_personal_dict(hspell_hash *personaldict,
		   hspell_hash *personaldict_new_words,
		   int currentdir_dictfile)
{
	FILE *fp;
	hspell_hash_keyvalue *new_words_array;
	int new_words_number, i;
	char dict_filename[512];

	char *home = getenv("HOME");
	if (currentdir_dictfile || !home)
		snprintf(dict_filename, sizeof(dict_filename),
			 "./hspell_words");
	else
		snprintf(dict_filename, sizeof(dict_filename),
			 "%s/.hspell_words", home);

	fp = fopen(dict_filename, "a");
	if (!fp)
		return 0; /* signal error */

	/* We append the new words to the file.
	   We also move them from personaldict_new_words to personaldict
	   so that subsequent calls to this function won't write them again
	   and again.
	*/
	/* NOTE: currently, we assume that the personal dictionary we
           originally read, or last wrote, is the current state of the
	   user's personal dictionary file. This may be wrong if several
	   hspell processes are running concurrently and adding words or the
	   user has been manually editing the file while hspell is running.
	   It might be safer, perhaps, to load the personal dictionary again
	   to see its really current state? In any case, the current
	   behavior isn't likely to cause any serious problems, just the
	   occasional words listed more than once, perhaps.
	*/
	new_words_array = hspell_hash_build_keyvalue_array(
		personaldict_new_words, &new_words_number);
	if (hspell_debug) {
		fprintf(stderr, "Saving %d words to %s\n",
				new_words_number, dict_filename);
	}
	for (i = 0; i < new_words_number; i++) {
		fprintf(fp, "%s\n", new_words_array[i].key);
		hspell_hash_incr_int(personaldict, new_words_array[i].key);
	}
	hspell_hash_free_keyvalue_array(personaldict_new_words,
			new_words_number, new_words_array);

	hspell_hash_destroy(personaldict_new_words);
	hspell_hash_init(personaldict_new_words);

	return (fclose(fp) == 0);
}

/* load_spelling_hints reads the spelling hints file (for the -n option).
   This is done in a somewhat ad-hoc manner.
*/

char *flathints;
int flathints_size;
void load_spelling_hints(hspell_hash *spellinghints) {
	FILE *fp;
	char s[1000];
	int len=0;
	int thishint=0;

	hspell_hash_init(spellinghints);

	flathints_size = 8192; /* initialize size (will grow as necessary) */
	flathints = (char *)malloc(flathints_size);
	/*flathints[0]=0;*/

	snprintf(s,sizeof(s),"gzip -dc '%s.hints'",
		 hspell_get_dictionary_path());
	fp = popen(s, "r");
	if(!fp) {
		fprintf(stderr,"Failed to open %s\n",s);
		return;
	}
	while(fgets(s, sizeof(s), fp)){
		int l=strlen(s);
		if(s[0]=='+') { /* this is a textual description line */
			if(!thishint){
				thishint=len;
			}
			/* reallocate the array, if no room */
			while(len+l >= flathints_size){
				flathints_size *= 2;
				flathints= (char *)
					realloc(flathints,flathints_size);
			}
			/* replace the '+' character by a space (this was
			   the way hints were printed in version 0.5, and
			   wee keep it for backward compatibility */
			s[0]=' ';
			/*strncpy(flathints+len, s, flathints_size-len);*/
			strcpy(flathints+len, s);
			len += l;
		} else if(s[0]=='\n'){ /* no more words for this hint */
			thishint = 0;
			len++;
		} else { /* another word for this hint */
			s[l-1]=0;
			hspell_hash_set_int(spellinghints, s, thishint);
		}
       }
       pclose(fp);
}


/* used for sorting later: */
static int
compare_key(const void *a, const void *b){
	register hspell_hash_keyvalue *aa = (hspell_hash_keyvalue *)a;
	register hspell_hash_keyvalue *bb = (hspell_hash_keyvalue *)b;
	return strcmp(aa->key, bb->key);
}
static int
compare_value_reverse(const void *a, const void *b){
	register hspell_hash_keyvalue *aa = (hspell_hash_keyvalue *)a;
	register hspell_hash_keyvalue *bb = (hspell_hash_keyvalue *)b;
	if(aa->value < bb->value)
		return 1;
	else if(aa->value > bb->value)
		return -1;
	else return 0;
}

static FILE *
next_file(int *argcp, char ***argvp)
{
	FILE *ret=0;
	if(*argcp<=0)
		return 0;
	while(*argcp && !ret){
		ret=fopen((*argvp)[0],"r");
		if(!ret)
			perror((*argvp)[0]);
		(*argvp)++;
		(*argcp)--;
	}
	return ret;
}


#define VERSION_IDENTIFICATION ("@(#) International Ispell Version 3.1.20 " \
			       "(but really Hspell/C %d.%d%s)\n")


/* ishebrew() checks for an intra-word Hebrew character. This includes the
   Hebrew alphabet and the niqqud characters. The 8-bit encoding that these
   characters may appear in is the "cp1255" encoding, Microsoft's extension
   to the iso-8859-8 standard (which did not contain niqqud). For the tables
   of these encodings, see
   http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1255.TXT
   http://www.unicode.org/Public/MAPPINGS/ISO8859/8859-8.TXT
 */
#define isniqqud(c) ((unsigned char)(c)>= 0xC0 && (unsigned char)(c) <= 0xD2 \
		     && (unsigned char)(c)!=0xCE && (unsigned char)(c)!=0xD0)
#define ishebrew(c) (((c)>=(int)(unsigned char)'א' && (c)<=(int)(unsigned char)'ת')||isniqqud(c))

static int uglyuglyflag = 0;

int notify_split(const char *w, const char *baseword, int preflen, int prefspec)
{
#ifdef USE_LINGINFO
	char *desc,*stem;
#endif
	if(preflen>0){
		printf("צירוף חוקי: %.*s+%s\n", preflen, w, baseword);
	} else if (!preflen){
		printf("מילה חוקית: %s\n",w);
	}
#ifdef USE_LINGINFO
	if (linginfo_lookup(baseword,&desc,&stem)) {
		int j;
		for (j=0; ;j++) {
			char buf[80];
			if (!linginfo_desc2text(buf, desc, j)) break;
			if (linginfo_desc2ps(desc, j) & prefspec) {
				printf("\t%s(%s%s)",linginfo_stem2text(stem,j),buf,uglyuglyflag ? ",##שגיאה##" : "");
				if (hspell_debug) printf("\t%d",linginfo_desc2ps(desc, j));
				printf("\n");
			}
		}
	}
#endif
	return 1;
}

int
main(int argc, char *argv[])
{
	struct dict_radix *dict;
#define MAXWORD 30
	char word[MAXWORD+1], *w;
	int wordlen=0, offset=0, wordstart;
	int c;
	int res;
	FILE *slavefp;
	int terse_mode=0;
	hspell_hash wrongwords;
	int preflen; /* used by -l */
	hspell_hash spellinghints;

	/* Following Ispell, we keep three lists of personal words to be
	   accepted: "personaldict" is the user's on-disk personal dictionary,
	   "personaldict_new_words" are words that the user asked to add to
	   the personal dictionary but which we haven't saved to disk yet,
	   and "sessiondict" are words that the user asked to accept during
	   this session, but not add to the on-disk personal dictionary.
	*/
	hspell_hash personaldict;
	hspell_hash personaldict_new_words;
	hspell_hash sessiondict;
	int currentdir_dictfile = 0;  /* file ./hspell_words exists? */

	/* command line options */
	char *progname=argv[0];
	int interpipe=0; /* pipe interface (ispell -a like) */
	int slave=0;  /* there's a slave ispell process (-i option) */
	int opt_s=0; /* -s option */
	int opt_c=0; /* -c option */
	int opt_l=0; /* -l option */
	int opt_v=0; /* -v option (show version and quit) */
	int opt_H=0; /* -H option (allow he ha-she'ela) */
	int opt_n=0; /* -n option (provide spelling hints) */

	/* TODO: when -a is not given, allow filename parameters, like
	   the "spell" command does. */
	FILE *in=stdin;

	/* Parse command-line options */
	while((c=getopt(argc, argv, "clnsviad:BmVhT:CSPp:w:W:HD:"))!=EOF){
		switch(c){
		case 'a':
			interpipe=1;
			break;
		case 'i':
			slave=1;
			break;
		/* The following options do something on ispell or aspell,
		   and some confused programs call hspell with them. We just
		   ignore them silently, hoping that all's going to be well...
		*/
		case 'd': case 'B': case 'm': case 'T': case 'C': case 'S':
		case 'P': case 'p': case 'w': case 'W':
			/*fprintf(stderr, "Warning: ispell options -d, -B and "
			  "-m are ignored by hspell.\n");*/
			break;
		case 's':
			opt_s=1;
			break;
		case 'c':
			opt_c=1;
			break;
		case 'l':
			opt_l=1;
			break;
		case 'H':
			/* Allow "he ha-she'ela" */
			opt_H=1;
			break;
		case 'n':
			opt_n=1;
			break;
		case 'v':
			opt_v++;
			break;
		case 'D':
			hspell_set_dictionary_path(optarg);
			break;
		case 'V':
			printf("Hspell %d.%d%s\nWritten by Nadav Har'El and "
			       "Dan Kenigsberg.\n\nCopyright (C) 2000-2017 "
			       "Nadav Har'El and Dan Kenigsberg.\nThis is "
			       "free software, released under the GNU Affero General "
			       "Public License\n(AGPL) version 3. See "
			       "http://hspell.ivrix.org.il/ for "
			       "more information.\n", HSPELL_VERSION_MAJOR,
			       HSPELL_VERSION_MINOR, HSPELL_VERSION_EXTRA);
			return 0;
		case 'h': case '?':
			fprintf(stderr,"hspell - Hebrew spellchecker\n"
				"Usage: %s [-acinslVH] [file ...]\n\n"
				"See hspell(1) manual for a description of "
				"hspell and its options.\nRun hspell -V for "
				"hspell's version and copyright.\n", progname);
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	/* The -v option causes ispell to print its current version
	   identification on the standard output and exit. If the switch is
	   doubled, ispell will also print the options that it was compiled
	   with.
	*/
	if(opt_v){
		printf(VERSION_IDENTIFICATION, HSPELL_VERSION_MAJOR,
		       HSPELL_VERSION_MINOR, HSPELL_VERSION_EXTRA);
		if (opt_v > 1) {
		    printf("Compiled-in options:\n");
		    printf("\tDICTFILE = \"%s\"\n", hspell_get_dictionary_path());
#ifdef USE_LINGINFO
		    printf("\tLINGINFO\n");
#endif
		}
		return 0;
	}

	/* If the program name ends with "-i", we enable the -i option.
	   This ugly hack is useful when a certain application can be given
	   a different spell-checker, but not extra options to pass to it */
	if(strlen(progname)>=2 && progname[strlen(progname)-2] == '-' &&
	   progname[strlen(progname)-1] == 'i'){
		slave=interpipe=1;
	}

	if(interpipe){
		/* for ispell -a like behavior, we want to flush every line: */
		setlinebuf(stdout);
	} else {
		/* No "-a" option: UNIX spell-like mode: */

		/* Set up hash-table for remembering the wrong words seen */
		hspell_hash_init(&wrongwords);

		/* If we have any more arguments, treat them as files to
		   spellcheck. Otherwise, just use stdin as set above.
		*/
		if(argc){
			in=next_file(&argc, &argv);
			if(!in)
				return 1; /* nothing to do, really... */
		}
	}

	if(hspell_init(&dict, (opt_H ? HSPELL_OPT_HE_SHEELA : 0) |
			      (opt_l ? HSPELL_OPT_LINGUISTICS : 0))<0){
		fprintf(stderr,"Sorry, could not read dictionary. Hspell "
			"was probably installed improperly.\n");
		return 1;
	}
	load_personal_dict(&personaldict, &currentdir_dictfile);
	hspell_hash_init(&personaldict_new_words);
	hspell_hash_init(&sessiondict);

	if(opt_n)
		load_spelling_hints(&spellinghints);

	if(interpipe){
		if(slave){
			/* We open a pipe to an "ispell -a" process, letting
			   it output directly to the user. We also let it
			   output its own version string instead of ours. Is
			   this wise? I don't know. Does anyone care?
			   Note that we also don't make any attempts to catch
			   broken pipes.
			*/
			slavefp=popen("ispell -a", "w");
			if(!slavefp){
				fprintf(stderr, "Warning: Cannot create slave "
				    "ispell process. Disabling -i option.\n");
				slave=0;
			} else {
				setlinebuf(slavefp);
			}
		}
		if(!slave)
			printf(VERSION_IDENTIFICATION, HSPELL_VERSION_MAJOR,
			       HSPELL_VERSION_MINOR, HSPELL_VERSION_EXTRA);
	}

	for(;;){
		c=getc(in);
		if(ishebrew(c) || c=='\'' || c=='"'){
			/* swallow up another letter into the word (if the word
			 * is too long, lose the last letters) */
			if(wordlen<MAXWORD)
				word[wordlen++]=c;
		} else if(wordlen){
			/* found word separator, after a non-empty word */
			word[wordlen]='\0';
			wordstart=offset-wordlen;
			/* TODO: convert two single quotes ('') into one
			 * double quote ("). For TeX junkies. */

			/* remove quotes from end or beginning of the word
			 * (we do leave, however, single quotes in the middle
			 * of the word - used to signify "j" sound in Hebrew,
			 * for example, and double quotes used to signify
			 * acronyms. A single quote at the end of the word is
			 * used to signify an abbreviation, or can be an actual
			 * quote (there is no difference in ASCII...), so we
			 * must check both possibilities. */
			w=word;
			if(*w=='"' || *w=='\''){
				w++; wordlen--; wordstart++;
			}
			if(w[wordlen-1]=='"'){
				w[wordlen-1]='\0'; wordlen--;
			}
			res=hspell_check_word(dict,w,&preflen);
			if(res!=1 && (res=hspell_is_canonic_gimatria(w))){
				if(hspell_debug)
					fprintf(stderr,"found canonic gimatria\n");
				if(opt_l){
					printf("גימטריה: %s=%d\n",w,res);
					preflen = -1; /* yes, I know it is bad programming, but I need to tell later printf not to print anything, and I hate to add a flag just for that. */
				}
				res=1;
			}
			if(res!=1 && w[wordlen-1]=='\''){
				/* try again, without the quote */
				w[wordlen-1]='\0'; wordlen--;
				res=hspell_check_word(dict,w,&preflen);
			}
			/* as last resort, try the user's personal word list */
			if(res!=1)
			   res = hspell_hash_exists(&personaldict, w)
			      || hspell_hash_exists(&personaldict_new_words, w)
			      || hspell_hash_exists(&sessiondict, w);

			if(res){
				if(hspell_debug)
					fprintf(stderr,"correct: %s\n",w);
				if(interpipe && !terse_mode)
					if(wordlen)
						printf("*\n");
				if(opt_l){
					hspell_enum_splits(dict,w,notify_split);
				}
			} else if(interpipe){
				/* Misspelling in -a mode: show suggested
				   corrections */
				struct corlist cl;
				int i;
				if(hspell_debug)
					fprintf(stderr,"misspelling: %s\n",w);
				corlist_init(&cl);
				hspell_trycorrect(dict, w, &cl);
				if(corlist_n(&cl))
					printf("& %s %d %d: ", w,
					       corlist_n(&cl), wordstart);
				else
					printf("# %s %d", w, wordstart);
				for(i=0;i<corlist_n(&cl);i++){
					printf("%s%s",
					       i ? ", " : "",
					       corlist_str(&cl,i));
				}
				printf("\n");
				corlist_free(&cl);
				if(opt_n){
					int index;
					if(hspell_hash_get_int(&spellinghints,
							       w, &index))
						printf("%s", flathints+index);
				}
			} else {
				/* Misspelling in "spell" mode: remember this
				   misspelling for later */

				if(hspell_debug)
					fprintf(stderr,"misspelling: %s\n",w);
				hspell_hash_incr_int(&wrongwords, w);
			}
			/* We treat the combination of the -l (linguistic
			   information) and -c (suggest corrections) option
			   as special. In that case we suggest "corrections"
			   to every word (regardless if they are in the
			   dictionary or not), and show the linguistic
			   information on all those words. This can be useful
			   for a reader application, which may also want to
			   be able to understand misspellings and their possible
			   meanings.
			*/
			if (opt_l && opt_c) {
				struct corlist cl;
				int i;
				if(hspell_debug)
					fprintf(stderr,"misspelling: %s\n",w);
				corlist_init(&cl);
				hspell_trycorrect(dict, w, &cl);
				uglyuglyflag = 1;
				for(i=0;i<corlist_n(&cl);i++){
					hspell_enum_splits(dict,corlist_str(&cl,i),notify_split);
				}
				uglyuglyflag = 0;
				corlist_free(&cl);
			}
			/* we're done with this word: */
			wordlen=0;
		} else if(interpipe &&
			  offset==0 && (c=='#' || c=='!' || c=='~' || c=='@' ||
					c=='%' || c=='-' || c=='+' || c=='&' ||
					c=='*')){
			/*
			   Summary of ispell's commands:
			   -----------------------------
			   ! - enter terse mode
			   % - exit terse mode

			   * <word> - add to personal dict
			   & <word> - ditto
			   @ <word> - accept, but leave out of dict
			   # - save personal dict
			*/
			char rest[512];
			int  isheb = 0;

			/* Read rest of line, to get the command parameters. */
			if(!fgets(rest, sizeof(rest), in)){
				rest[0]='\0'; /* unexpected EOF... */
			} else if (rest[0] && rest[strlen(rest)-1] == '\n') {
				rest[strlen(rest)-1] = '\0';
			} else {
				/* We shouldn't arrive here, but if we do:
				   Eat up rest of line. */
				int rc;
				while ((rc = getc(in)) != EOF && rc != '\n')
					;
			}

			switch (c) {
			case '!': terse_mode = 1; break;
			case '%': terse_mode = 0; break;
			case '*': case '&': case '@':
				isheb = ishebrew((int)(unsigned char)rest[0]);
				/* We don't handle non-Hebrew words */
				if (isheb) {
					if (c == '@') {
					/* Add word to the session dictionary,
					   which is never saved to disk. */
					  if (hspell_debug)
					    fprintf(stderr, "hspell_add_to_session(%s)\n", rest);
					  hspell_hash_incr_int(
					    &sessiondict, rest);
					} else {
					/* Add word to personaldict_new_words,
					   which is saved to disk when the '#'
					   command is issued. */
					   if (hspell_debug)
					     fprintf(stderr, "hspell_add_to_personal(%s)\n", rest);
					   if (!hspell_hash_exists(
					     &personaldict, rest) &&
					       !hspell_hash_exists(
					     &personaldict_new_words, rest)) {
					     hspell_hash_incr_int(
                                                &personaldict_new_words, rest);
					   }
					}
				}
				break;
			case '#':
				save_personal_dict(&personaldict,
						   &personaldict_new_words,
						   currentdir_dictfile);
				break;
			}

			/* Pass the command to ispell only if it
			   doesn't involve a Hebrew word. */
			if (slave && !isheb) {
				fprintf(slavefp, "%c%s\n", c, rest);
			}
			/* offset=0 remains but we don't want to output
			   a newline */
			continue;
		}
		if(c==EOF) {
			/* If we were in the middle of the line (no newline)
			   we nevertheless need to finish with the old line */
			if(offset){
				offset=0;
				if(interpipe && !slave)
					printf("\n");
			}
			/* in UNIX spell mode (!interpipe) we should read
			   all the files given in the command line...
			   Otherwise, an EOF is the end of this loop.
			*/
			if(!interpipe && argc>0){
				if(in!=stdin)
					fclose(in);
				in=next_file(&argc, &argv);
				if(!in)
					break;
			} else
				break;
		}
		if(c=='\n'){
			offset=0;
			if(interpipe && !slave)  /*slave already outputs a newline...*/
			printf("\n");
		} else {
			offset++;
		}
		/* pass the character also to the slave, replacing Hebrew
		   characters by spaces */
		if(interpipe && slave)
			putc(ishebrew(c) ? ' ' : c, slavefp);
	}

	/* in spell-like mode (!interpipe) - list the wrong words */
	if(!interpipe){
		hspell_hash_keyvalue *wrongwords_array;
		int wrongwords_number;
		wrongwords_array = hspell_hash_build_keyvalue_array(
			&wrongwords, &wrongwords_number);

		if(wrongwords_number){
			int i;
			if(opt_c)
				printf("שגיאות כתיב שנמצאו, ותיקוניהן "
				       "המומלצים:\n\n");
			else
				printf("שגיאות כתיב שנמצאו:\n\n");

			/* sort word list by key or value (depending on -s
			   option) */
			qsort(wrongwords_array, wrongwords_number,
			      sizeof(hspell_hash_keyvalue),
			      opt_s ? compare_value_reverse : compare_key);

			for(i=0; i<wrongwords_number; i++){
				if(opt_c){
					struct corlist cl;
					int j;
					printf("%d %s -> ",
					       (int)wrongwords_array[i].value,
					       wrongwords_array[i].key);
					corlist_init(&cl);
					hspell_trycorrect(dict,
					       wrongwords_array[i].key, &cl);
					for(j=0;j<corlist_n(&cl);j++){
						printf("%s%s",
						       j ? ", " : "",
						       corlist_str(&cl,j));
					}
					corlist_free(&cl);
					printf("\n");
				} else if(opt_s){
					printf("%d %s\n",
					       (int)wrongwords_array[i].value,
					       wrongwords_array[i].key);
				} else {
					printf("%s\n",wrongwords_array[i].key);
				}
				if(opt_n){
					int index;
					if(hspell_hash_get_int(&spellinghints,
					     wrongwords_array[i].key, &index))
						printf("%s", flathints+index);
				}
			}
		}
#if 0
		hspell_hash_free_keyvalue_array(&wrongwords, wrongwords_number,
						wrongwords_array);
#endif
	}

	return 0;
}
