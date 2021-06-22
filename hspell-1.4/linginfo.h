/* Copyright (C) 2003 Nadav Har'El and Dan Kenigsberg */

#ifndef INCLUDED_LINGINFO_H
#define INCLUDED_LINGINFO_H

#include "hspell.h"

/* load description and stem files into memory */
int linginfo_init(const char *dir);

/* free'em */
int linginfo_free(void);

/* translate the i'th description of a word into human-readable text */
char *linginfo_desc2text(char *text, const char *desc, int i);

/* translate the i'th description of a word into older-style prefix specifier,
 * such as the PS_* that are kept in .prefixes */
int linginfo_desc2ps(const char *desc, int i);

/* translate the i'th stem-index of a word into human-readable text */
char *linginfo_stem2text(const char *stem, int i);

/* search for a word in the linginfo database. if a the word is found, fill the
 * pointed desc and stem buffers with the relevant (opaque) data. */
int linginfo_lookup(const char *word, char **desc, char **stem);

	
#endif /* INCLUDED_LINGINFO_H */
