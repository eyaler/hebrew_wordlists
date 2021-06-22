/* Copyright (C) 2003-2004 Nadav Har'El and Dan Kenigsberg */

#ifndef INCLUDED_HASH_H
#define INCLUDED_HASH_H

/* we use tclHash.[ch] to implement the following API:

   typedef ...hspell_hash;
   void hspell_hash_init(hspell_hash *p);
   void hspell_hash_incr_int(hspell_hash *hashp, const char *key);

   int hspell_hash_exists(hspell_hash *hashp, const char *key);
   int hspell_hash_get_int(hspell_hash *hashp, const char *key, int *value);
   void hspell_hash_set_int(hspell_hash *hashp, const char *key, int value);
   void hspell_hash_destroy(hspell_hash *p);


   typedef struct {
      const char *key;
      int value;
   } hspell_hash_keyvalue;
   hspell_hash_keyvalue *hspell_hash_build_keyvalue_array(hspell_hash *h,
                                                          int *size);
   void hspell_hash_free_keyvalue_array(hspell_hash *h, int size,
                                        hspell_hash_keyvalue *p);
*/

#include <stdlib.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "tclHash.h"

typedef Tcl_HashTable hspell_hash;

static inline void hspell_hash_init(hspell_hash *p)
{
	Tcl_InitHashTable(p, TCL_STRING_KEYS);
}

/* hspell_hash_incr_int increments the integer value (we assume) stored
   for the key. If there is no value for this key yet, it is initialized
   to zero and then incremented to 1.
*/
static inline void hspell_hash_incr_int(hspell_hash *hashp, const char *key)
{
	Tcl_HashEntry *e;
	int isnew;

	e=Tcl_CreateHashEntry(hashp, key, &isnew);

	/* Increment the value, as a an integer. We don't to cast the
	   address of clientData to another type in fear we'll write too
	   much, so we need explicit assignment and cast of the values:
	*/
	Tcl_SetHashValue(e, ((intptr_t)Tcl_GetHashValue(e))+1);
}

/* hspell_hash_exists returns 0 if there is no value for this key yet, 1
   otherwise.
*/
static inline int hspell_hash_exists(hspell_hash *hashp, const char *key)
{
	Tcl_HashEntry *e;
	e=Tcl_FindHashEntry(hashp, key);
	return e ? 1 : 0;
}

typedef struct {
	const char *key;
	intptr_t value;
} hspell_hash_keyvalue;

/* Hspell_hash_build_keyvalue_vector builds an array of keys and values
   from the given hash table.  Note that the keys are pointers to strings
   that sit inside the hash table, so they are only valid until the next
   time some key is deleted from the hash-table (or the hash table itself
   is deleted).
   This function return a pointer which the caller should free with
   hspell_hash_free_keyvalue_array().
*/
static inline hspell_hash_keyvalue *hspell_hash_build_keyvalue_array(
	hspell_hash *h, int *size)
{
	Tcl_HashEntry *e;
	Tcl_HashSearch s;
	hspell_hash_keyvalue *array, *arrayp, *arrayend;

	if(!h->numEntries){
		*size=0;
		return 0;
	}

	array=(hspell_hash_keyvalue *)
		malloc(h->numEntries*sizeof(hspell_hash_keyvalue));

	arrayp=array; /* moving pointer */
	arrayend=array+h->numEntries; /* pointer past end */

	for(e=Tcl_FirstHashEntry(h, &s); e; e=Tcl_NextHashEntry(&s)){
		if(arrayp>=arrayend){
			/* this cannot happen... */
			fprintf(stderr, "Internal error: allocated array of "
				"incorrect size. Truncating it.\n");
			break;
		}
		arrayp->key=Tcl_GetHashKey(h, e);
		arrayp->value=(intptr_t)Tcl_GetHashValue(e);
		arrayp++;
	}
	if(arrayp!=arrayend){
		/* this cannot happen... */
		fprintf(stderr, "Internal error: allocated array of incorrect"
			" size. Wasted space.\n");
		*size=(arrayp-array);
	} else
		*size=h->numEntries;

	return array;
}

static inline void hspell_hash_free_keyvalue_array(hspell_hash *h, int size,
					           hspell_hash_keyvalue *p)
{
	if(p)
		free(p);
}

/* The following functions also keep integer values in the hash table.
   These values must be small enough to fit in the flatform's pointer
   (on modern machines, this is not a problem - pointers are usually
   as large, or even larger, than integers). These functions are useful for
   Hspell's "-n" option, for example.
   The get function returns 0 on failure, and 1 on success with the value
   put in the given pointer.
*/
static inline intptr_t hspell_hash_get_int(hspell_hash *hashp, const char *key,
			              int *value)
{
	Tcl_HashEntry *e;

	if(!(e=Tcl_FindHashEntry(hashp, key)))
		return 0;
	*value=(intptr_t)Tcl_GetHashValue(e);
	return 1;
}

static inline void hspell_hash_set_int(hspell_hash *hashp, const char *key,
				       intptr_t value)
{
	Tcl_HashEntry *e;
	int isnew;

	e=Tcl_CreateHashEntry(hashp, key, &isnew);
	Tcl_SetHashValue(e, value);
}

/* After calling hspell_hash_destroy on p, p should not be used again
   until hspell_hash_init is used to create a new hash table.
*/
static inline void hspell_hash_destroy(hspell_hash *p)
{
	Tcl_DeleteHashTable(p);
}

#endif /* INCLUDED_HASH_H */
