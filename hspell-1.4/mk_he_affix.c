/* Copyright 2004-2012 Nadav Har'El and Dan Kenigsberg */

/* this little program creates hunspell or aspell dictionaries for Hebrew
 * according to the hebrew.wgz*.
 * We create a single rule for each of hspell's "word specifier". Each rule
 * expands to all the prefixes that provide that specifier (and the null
 * prefix is implied and NEEDAFFIX is specified for each word where this is
 * not appropriate).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "prefixes.c"
#include "hspell.h"

#define PREFIXFILE_COMMAND "gzip -dc hebrew.wgz.prefixes | ./specfilter"

/* Convert a number in the range 0..52 (currently) to a readable character
   that can be used as the rule (prefix set) name. To facilitate merging our
   word list with an English one (for spell-checking mixed text in software
   that does not support multiple word lists), we do not use the upper-case
   latin characters. Currently we use the lower-case letters, in addition to
   Hebrew characters (aspell and myspell have no problems with non-ascii
   characters) - but almost any symbols can be used to.
   A note for future expansion: Aspell has problems with a backslash, while
   Myspell works with them - so we will have to skip the backslash character
   if we use symbols. But with the digits and other symbols, there's plenty
   of room for future expansion.
*/
static inline char num_to_char(int i)
{
	if(i<0){
		fprintf(stderr,"internal error: num_to_char(%d)\n",i);
		exit(1);
	} else if(i<26){
		return 'a'+i;
	} else if(i<52){
		return 'א'+(i-26);
	} else {
		fprintf(stderr,"internal error: num_to_char(%d) ran out of symbols\n",i);
		exit(1);
	}
}

/* Usage: mk_he_affix <hunspell> <affixfile> <dictfile>
 * Where <hunspell> is: 0 for aspell, 1 for hunspell. Hunspell and aspell have
 * some different affix file features, and also different encoding requirements
 * (aspell requires ISO-8859-8, while hunspell is, for an unknown reason,
 * 10 times faster if we give it UTF-8).
 */
int main(int argc, char *argv[])
{
  int i, specifier;
  char seen_specifiers[100], rulechar;
  int already_seen=0, seen, count;
  char needaffix=0;
  FILE *prefixfp, *wordsfp;
  FILE *afffp, *dicfp;
  int prefixes_size = 0;
  char *prefix_is_word;
  int hunspell;

  if(argc!=4){
    fprintf(stderr,"%d\n",argc);
    fprintf(stderr,"Usage: %s <hunspell> <affixfile> <dictfile>\n", argv[0]);
    exit(1);
  }
  hunspell=atoi(argv[1]);


  if(hunspell){
    char s[256];
    /* Unfortunately, the dictionary file should start with an approximate
     * count of the number of words. Note that this count is only approximate
     * as we also add a list of stand-alone prefixes at the end.
     */
    snprintf(s, sizeof(s), "gzip -dc hebrew.wgz | ./wunzip | wc -l > %s", argv[3]);
    system(s);
    snprintf(s, sizeof(s), "iconv -f iso-8859-8 -t utf-8 >%s", argv[2]);
    afffp = popen(s, "w");
    snprintf(s, sizeof(s), "iconv -f iso-8859-8 -t utf-8 >>%s", argv[3]);
    dicfp = popen(s, "w");
  } else {
    afffp = fopen(argv[2], "w");
    dicfp = fopen(argv[3], "w");
  }

  fprintf(afffp, "# This file was generated automatically from data prepared\n"
                 "# by the Hspell project (http://hspell.ivrix.org.il/).\n"
                 "# Hspell version %d.%d%s was used.\n",
                 HSPELL_VERSION_MAJOR,HSPELL_VERSION_MINOR,HSPELL_VERSION_EXTRA);
  fprintf(afffp, "# Copyright 2004-2017, Nadav Har'El and Dan Kenigsberg\n");
  fprintf(afffp, "# The dictionary (this file and the corresponding word list)\n"
                 "# is licensed under the GNU Affero General Public License\n"
		 "# (AGPL) version 3.\n");

  if(hunspell){
	fprintf(afffp,
	  "SET UTF-8\n"
	  "TRY יוהאעחכק'\"שסזדגברנמטצתפםףךץןל\n"
	  "WORDCHARS אבגדהוזחטיכלמנסעפצקרשתםןךףץ'\"\n"
	  "BREAK 3\n"
	  "BREAK ^\"\n"
	  "BREAK \"$\n"
	  "BREAK ^'\n"
	  "MAP 10\n"
	  "MAP ךכח\n"
	  "MAP םמ\n"
	  "MAP ןנ\n"
	  "MAP ףפ\n"
	  "MAP ץצ\n"
	  "MAP כק\n"
	  "MAP אע # for English\n"
	  "MAP גה # for Russian\n"
	  "MAP צס # for Arabic\n"
	  "MAP חכר # for French\n"
        );
  }

  prefixfp = popen(PREFIXFILE_COMMAND, "r");
  while ((specifier=fgetc(prefixfp))!= EOF) {
    for(i=0, seen=0; (i<already_seen) && !seen; i++) {
      if (seen_specifiers[i] == specifier) seen = 1; }
    if (seen) continue;
    seen_specifiers[already_seen++] = specifier;

    /* count the number of matching prefixes */
    for (i=1, count=0; prefixes_noH[i]!=0; i++) {
      if (masks_noH[i] & specifier) {
        if (!strcmp("ו",prefixes_noH[i])) count += 2;
        else count += 4;
      }
    }

    rulechar = num_to_char(already_seen-1);
    fprintf(afffp, "PFX %c N %d\n",rulechar,count);

    /* print one rule for each legal prefix that goes with this word type,
     * and remember to double initial waw if a prefix is prepended.
     *
     * The empty prefix, prefixes_nohH[0], needs special treatment. While
     * other allowed prefixes need to be explictly added to the rules (as we
     * do below), the empty prefix is by default allowed, and if it is not
     * desired we need to explicitly disallow it with a special flag on
     * every word for which we don't want to allow the empty prefix, with a
     * special NEEDAFFIX flag.
     * Unfortunately, NEEDAFFIX is only supported by hunspell; Aspell ignores
     * it, and therefore mistakenly accepts the maqor natuy without a prefix,
     * e.g., ישון, as in לישון but without the prefix.
     */
    if (!(masks_noH[0] & specifier)){
      /* Too bad this isn't supported because only one NEEDAFFIX allowed.
       * So we'll need to have a single NEEDAFFIX flag, and specify it on
       * individual words that need it
       */
      /* fprintf(afffp, "NEEDAFFIX %c\n",rulechar); */
      needaffix=1;
    }
    for (i=1; prefixes_noH[i]!=0; i++) {
      if (masks_noH[i] & specifier) {
        if (!strcmp("ו",prefixes_noH[i])) {
          fprintf(afffp, "PFX %c 0 %s .\n",rulechar,prefixes_noH[i]);
          fprintf(afffp, "PFX %c 0 %s\" .\n",rulechar,prefixes_noH[i]);
        } else {
          fprintf(afffp, "PFX %c 0 %s [^ו]\n",rulechar,prefixes_noH[i]);
          fprintf(afffp, "PFX %c 0 %s וו\n",rulechar,prefixes_noH[i]);
          fprintf(afffp, "PFX %c 0 %s\" .\n",rulechar,prefixes_noH[i]);
          fprintf(afffp, "PFX %c 0 %sו ו[^ו]\n",rulechar,prefixes_noH[i]);
        }
      }
    }
    prefixes_size = i;
    fprintf(afffp, "\n");
  }
  if (hunspell && needaffix) {
    needaffix = num_to_char(already_seen);
    fprintf(afffp, "NEEDAFFIX %c\n",needaffix);
  }
  pclose(prefixfp);
  if(hunspell)
    pclose(afffp);
  else
    fclose(afffp);

  prefix_is_word = (char *)calloc(sizeof(char),prefixes_size);

  /* and now, translate hebrew.wgz+hebrew.wgz.prefix into aspell-style word
   * list. */

  prefixfp = popen(PREFIXFILE_COMMAND, "r");
  wordsfp = popen("gzip -dc hebrew.wgz|./wunzip", "r");

  while ((specifier=fgetc(prefixfp))!= EOF) {
    char word[100];
    int j;
    /* find the specifier place (which infers which aspell rule apply to its
     * word) */
    for(i=0; (i<already_seen) && (seen_specifiers[i]!=specifier) ; i++);
    fgets(word, sizeof(word)-3,wordsfp);

    /* write down whether this word is also a legal prefix (and therefore should
       not be written again later)  */
    for (j=1; prefixes_noH[j]!=0; j++) {
      if (!strcmp(word,prefixes_noH[j])) {
        if (masks_noH[0] & specifier) /* this word is allowed on its own */
          prefix_is_word[j] = 1;
        break;
      }
    }

    word[strlen(word)-1]='\0'; /* remove trailing newline */
    fprintf(dicfp,"%s",word);
    putc('/', dicfp);
    putc(num_to_char(i), dicfp);
    if (hunspell && !(masks_noH[0] & specifier))
      /* because we can't specify NEEDAFFIX for several prefixes, unfortunately
       * we need to use one ("needaffix") and put it on individual words */
      putc(needaffix, dicfp);
    putc('\n', dicfp);
  }
  pclose(prefixfp);
  pclose(wordsfp);

  /* accept "dangling" prefixes, that many times precede numbers and latin */
  /* but make sure not to repeat words that already appear in the dictionary.
   * This may cause unwanted warning. */
  for (i=1; prefixes_noH[i]!=0; i++) {
    if (!prefix_is_word[i])
      fprintf(dicfp, "%s\n", prefixes_noH[i]);
  }
  free(prefix_is_word);

  if(hunspell)
    pclose(dicfp);
  else
    fclose(dicfp);
  return 0;
}

