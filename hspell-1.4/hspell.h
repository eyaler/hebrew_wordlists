/* Copyright (C) 2003-2017 Nadav Har'El and Dan Kenigsberg */

/* This header file defines the Hspell Hebrew spellchecking API in C, as
   implemented by the libhspell.a library.
   Please check out the hspell(3) manual for more information on how to use
   the Hspell C interface.
*/
#ifndef INCLUDED_HSPELL_H
#define INCLUDED_HSPELL_H

/* The following macros can be used to verify which version of the Hspell
   API this header file supports. Note that this API might change.
*/
#define HSPELL_VERSION_MAJOR 1
#define HSPELL_VERSION_MINOR 4
#define HSPELL_VERSION_EXTRA ""


struct dict_radix;
struct corlist;

/* flags for hspell_init: */
#define HSPELL_OPT_DEFAULT 0
#define HSPELL_OPT_HE_SHEELA 1      /* flag to accept He Ha-she'ela */
#define HSPELL_OPT_LINGUISTICS  2   /* initialize morphological analyzer,
				       not just spell-checker */

int hspell_init(struct dict_radix **dictp, int flags);

int hspell_check_word(struct dict_radix *dict,
		      const char *word, int *preflen);
void hspell_trycorrect(struct dict_radix *dict,
		       const char *w, struct corlist *cl);
unsigned int hspell_is_canonic_gimatria(const char *w);

void hspell_uninit(struct dict_radix *dict);

const char *hspell_get_dictionary_path(void);
void hspell_set_dictionary_path(const char *path);

extern int hspell_debug;

/* Corlist is our simple data structure for holding a list of corrections
 * returned by hspell_trycorrect. This silly implementation has fixed sizes!
 * A no-no in good programming, but enough for what we need it for... (the
 * implementation makes sure that the arrays aren't overflowed, don't worry).
 */
#define N_CORLIST_WORDS 50
#define N_CORLIST_LEN 30    /* max len per word */
struct corlist {
	char correction[N_CORLIST_WORDS][N_CORLIST_LEN];
	int n;
};
int corlist_add(struct corlist *cl, const char *s);
int corlist_init(struct corlist *cl);
int corlist_free(struct corlist *cl);


#define corlist_n(cl) ((cl)->n)
#define corlist_str(cl,i) ((cl)->correction[(i)])

/* type definition for the function to be called by hspell_enum_splits on
   every legal split between prefix and base word that is found. word is the
   original word that is split. baseword is the base word found, preflen is
   the length of the prefix, and prefspec is the prefix specifier of the base
   word.
*/
typedef int hspell_word_split_callback_func(const char *word,
	    const char *baseword, int preflen, int prefspec);

/* find all legal splittings of word into a baseword and a prefix. call enumf
 * for every such split. */
int hspell_enum_splits(struct dict_radix *dict, const char *word,
	hspell_word_split_callback_func *enumf);

#endif /* INCLUDED_HSPELL_H */
