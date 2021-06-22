/* Copyright (C) 2003 Nadav Har'El and Dan Kenigsberg */

#ifndef INCLUDED_RADIX_H
#define INCLUDED_RADIX_H

/* The following structure is opaque for the user - its fields can only
   be accessed by calling functions, and it can only be instantiated as
   a pointer (by calling new_dict_radix).
   This is object-oriented programming in C :)
*/
struct dict_radix;

struct dict_radix *new_dict_radix(void);
void delete_dict_radix(struct dict_radix *dict);
int allocate_nodes(struct dict_radix *dict, int nsmall, int nmedium, int nfull);

int read_dict(struct dict_radix *dict, const char *dir);
void print_tree(struct dict_radix *dict);
void print_sizes(struct dict_radix *dict);
void print_stats(struct dict_radix *dict);

int lookup(const struct dict_radix *dict, const char *word);

#endif /* INCLUDED_RADIX_H */
