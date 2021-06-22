/* The following file was taken from TCL 8.4.3's tcl8.4.3/generic/tclHash.c,
   with the following necessary modifications made by Nadav Har'El:
     * the "license.terms" file from that distribution is included in a
       comment below
     * RCS line removed (our RCS interfered with it)
     * Assume TCL_PRESERVE_BINARY_COMPATABILITY is 0, 
       TCL_HASH_KEY_STORE_HASH is 1, and remove #ifs  related to it.
       Also removed #if 0 stuff.
     * copy stuff from generic/tcl.h
     * Removed key types other than string, and statistics routines.
     * commented out panic-related lines.
     * Other changes required to compile without warnings on modern systems.
*/

#define ckfree free
#define ckalloc malloc
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif


/* 
 * tclHash.c --
 *
 *	Implementation of in-memory hash tables for Tcl and Tcl-based
 *	applications.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* The contents of the license.terms file:

   This software is copyrighted by the Regents of the University of
   California, Sun Microsystems, Inc., Scriptics Corporation, ActiveState
   Corporation and other parties.  The following terms apply to all files
   associated with the software unless explicitly disclaimed in
   individual files.

   The authors hereby grant permission to use, copy, modify, distribute,
   and license this software and its documentation for any purpose, provided
   that existing copyright notices are retained in all copies and that this
   notice is included verbatim in any distributions. No written agreement,
   license, or royalty fee is required for any of the authorized uses.
   Modifications to this software may be copyrighted by their authors
   and need not follow the licensing terms described here, provided that
   the new terms are clearly indicated on the first page of each file where
   they apply.

   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
   IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
   NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.

   GOVERNMENT USE: If you are acquiring this software on behalf of the
   U.S. government, the Government shall have only "Restricted Rights"
   in the software and related documentation as defined in the Federal 
   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
   are acquiring the software on behalf of the Department of Defense, the
   software shall be classified as "Commercial Computer Software" and the
   Government shall have only "Restricted Rights" as defined in Clause
   252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
   authors grant the U.S. Government and others acting in its behalf
   permission to use and distribute the software in accordance with the
   terms specified in this license. 
*/

#include "tclHash.h"


/*
 * Structure definition for the methods associated with a hash table
 * key type.
 */
#define TCL_HASH_KEY_TYPE_VERSION 1
struct Tcl_HashKeyType {
    int version;                /* Version of the table. If this structure is
                                 * extended in future then the version can be
                                 * used to distinguish between different
                                 * structures. 
                                 */

    int flags;                  /* Flags, see above for details. */

    /* Calculates a hash value for the key. If this is NULL then the pointer
     * itself is used as a hash value.
     */
    Tcl_HashKeyProc *hashKeyProc;

    /* Compares two keys and returns zero if they do not match, and non-zero
     * if they do. If this is NULL then the pointers are compared.
     */
    Tcl_CompareHashKeysProc *compareKeysProc;

    /* Called to allocate memory for a new entry, i.e. if the key is a
     * string then this could allocate a single block which contains enough
     * space for both the entry and the string. Only the key field of the
     * allocated Tcl_HashEntry structure needs to be filled in. If something
     * else needs to be done to the key, i.e. incrementing a reference count
     * then that should be done by this function. If this is NULL then Tcl_Alloc
     * is used to allocate enough space for a Tcl_HashEntry and the key pointer
     * is assigned to key.oneWordValue.
     */
    Tcl_AllocHashEntryProc *allocEntryProc;

    /* Called to free memory associated with an entry. If something else needs
     * to be done to the key, i.e. decrementing a reference count then that
     * should be done by this function. If this is NULL then Tcl_Free is used
     * to free the Tcl_HashEntry.
     */
    Tcl_FreeHashEntryProc *freeEntryProc;
};




/*
 * When there are this many entries per bucket, on average, rebuild
 * the hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3

/*
 * The following macro takes a preliminary integer hash value and
 * produces an index into a hash tables bucket list.  The idea is
 * to make it so that preliminary values that are arbitrarily similar
 * will end up in different buckets.  The hash function was taken
 * from a random-number generator.
 */

#define RANDOM_INDEX(tablePtr, i) \
    (((((long) (i))*1103515245) >> (tablePtr)->downShift) & (tablePtr)->mask)

/*
 * Prototypes for the string hash key methods.
 */

static Tcl_HashEntry *	AllocStringEntry _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));
static int		CompareStringKeys _ANSI_ARGS_((
			    VOID *keyPtr, Tcl_HashEntry *hPtr));
static unsigned int	HashStringKey _ANSI_ARGS_((
			    Tcl_HashTable *tablePtr,
			    VOID *keyPtr));

/*
 * Procedure prototypes for static procedures in this file:
 */

static void		RebuildTable _ANSI_ARGS_((Tcl_HashTable *tablePtr));

Tcl_HashKeyType tclStringHashKeyType = {
    TCL_HASH_KEY_TYPE_VERSION,		/* version */
    0,					/* flags */
    HashStringKey,			/* hashKeyProc */
    CompareStringKeys,			/* compareKeysProc */
    AllocStringEntry,			/* allocEntryProc */
    NULL				/* freeEntryProc */
};


/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare
 *	the hash table for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to Tcl_FindHashEntry and
 *	Tcl_CreateHashEntry.
 *
 *----------------------------------------------------------------------
 */

#undef Tcl_InitHashTable
void
Tcl_InitHashTable(tablePtr, keyType)
    register Tcl_HashTable *tablePtr;	/* Pointer to table record, which
					 * is supplied by the caller. */
    int keyType;			/* Type of keys to use in table:
					 * TCL_STRING_KEYS, TCL_ONE_WORD_KEYS,
					 * or an integer >= 2. */
{
#if (TCL_SMALL_HASH_TABLE != 4) 
    panic("Tcl_InitCustomHashTable: TCL_SMALL_HASH_TABLE is %d, not 4\n",
	    TCL_SMALL_HASH_TABLE);
#endif
    
    tablePtr->buckets = tablePtr->staticBuckets;
    tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
    tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
    tablePtr->numBuckets = TCL_SMALL_HASH_TABLE;
    tablePtr->numEntries = 0;
    tablePtr->rebuildSize = TCL_SMALL_HASH_TABLE*REBUILD_MULTIPLIER;
    tablePtr->downShift = 28;
    tablePtr->mask = 3;
    tablePtr->keyType = keyType;
    /*
     * Use the key type to decide which key type is needed.
     */
    if (keyType == TCL_STRING_KEYS) {
	    tablePtr->typePtr = &tclStringHashKeyType;
    } else {
	    /* TODO: get rid of this else. we have no other types */
	    abort();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FindHashEntry --
 *
 *	Given a hash table find the entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the
 *	hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_FindHashEntry(tablePtr, key)
    Tcl_HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find matching entry. */
{
    register Tcl_HashEntry *hPtr;
    Tcl_HashKeyType *typePtr;
    unsigned int hash;
    int index;

    typePtr = tablePtr->typePtr;
    if (typePtr == NULL) {
/*	Tcl_Panic("called Tcl_FindHashEntry on deleted table"); */
	return NULL;
    }

    if (typePtr->hashKeyProc) {
	hash = typePtr->hashKeyProc (tablePtr, (VOID *) key);
	if (typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
	    index = RANDOM_INDEX (tablePtr, hash);
	} else {
	    index = hash & tablePtr->mask;
	}
    } else {
	hash = (unsigned int) (intptr_t) key;
	index = RANDOM_INDEX (tablePtr, hash);
    }

    /*
     * Search all of the entries in the appropriate bucket.
     */

    if (typePtr->compareKeysProc) {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
	    if (typePtr->compareKeysProc ((VOID *) key, hPtr)) {
		return hPtr;
	    }
	}
    } else {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
	    if (key == hPtr->key.oneWordValue) {
		return hPtr;
	    }
	}
    }
    
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateHashEntry --
 *
 *	Given a hash table with string keys, and a string key, find
 *	the entry with a matching key.  If there is no matching entry,
 *	then create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this
 *	is a newly-created entry, then *newPtr will be set to a non-zero
 *	value;  otherwise *newPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_CreateHashEntry(tablePtr, key, newPtr)
    Tcl_HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find or create matching
				 * entry. */
    int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
{
    register Tcl_HashEntry *hPtr;
    Tcl_HashKeyType *typePtr;
    unsigned int hash;
    int index;

    typePtr = tablePtr->typePtr;
    if (typePtr == NULL) {
/*	Tcl_Panic("called Tcl_CreateHashEntry on deleted table"); */
	return NULL;
    }

    if (typePtr->hashKeyProc) {
	hash = typePtr->hashKeyProc (tablePtr, (VOID *) key);
	if (typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
	    index = RANDOM_INDEX (tablePtr, hash);
	} else {
	    index = hash & tablePtr->mask;
	}
    } else {
	hash = (unsigned int) (intptr_t) key;
	index = RANDOM_INDEX (tablePtr, hash);
    }

    /*
     * Search all of the entries in the appropriate bucket.
     */

    if (typePtr->compareKeysProc) {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
	    if (typePtr->compareKeysProc ((VOID *) key, hPtr)) {
		*newPtr = 0;
		return hPtr;
	    }
	}
    } else {
	for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	        hPtr = hPtr->nextPtr) {
	    if (hash != (unsigned int) hPtr->hash) {
		continue;
	    }
	    if (key == hPtr->key.oneWordValue) {
		*newPtr = 0;
		return hPtr;
	    }
	}
    }

    /*
     * Entry not found.  Add a new one to the bucket.
     */

    *newPtr = 1;
    if (typePtr->allocEntryProc) {
	hPtr = typePtr->allocEntryProc (tablePtr, (VOID *) key);
    } else {
	hPtr = (Tcl_HashEntry *) ckalloc((unsigned) sizeof(Tcl_HashEntry));
	hPtr->key.oneWordValue = (char *) key;
    }
					 
    hPtr->tablePtr = tablePtr;
    hPtr->hash = hash;
    hPtr->nextPtr = tablePtr->buckets[index];
    tablePtr->buckets[index] = hPtr;
    hPtr->clientData = 0;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many
     * more buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
	RebuildTable(tablePtr);
    }
    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteHashEntry --
 *
 *	Remove a single entry from a hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entry given by entryPtr is deleted from its table and
 *	should never again be used by the caller.  It is up to the
 *	caller to free the clientData field of the entry, if that
 *	is relevant.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteHashEntry(entryPtr)
    Tcl_HashEntry *entryPtr;
{
    register Tcl_HashEntry *prevPtr;
    Tcl_HashKeyType *typePtr;
    Tcl_HashTable *tablePtr;
    Tcl_HashEntry **bucketPtr;
    int index;

    tablePtr = entryPtr->tablePtr;
    typePtr = tablePtr->typePtr;

    
    if (typePtr->hashKeyProc == NULL
	|| typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
	index = RANDOM_INDEX (tablePtr, entryPtr->hash);
    } else {
	index = ((unsigned int) entryPtr->hash) & tablePtr->mask;
    }

    bucketPtr = &(tablePtr->buckets[index]);
    
    if (*bucketPtr == entryPtr) {
	*bucketPtr = entryPtr->nextPtr;
    } else {
	for (prevPtr = *bucketPtr; ; prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
/*		panic("malformed bucket chain in Tcl_DeleteHashEntry"); */
	    }
	    if (prevPtr->nextPtr == entryPtr) {
		prevPtr->nextPtr = entryPtr->nextPtr;
		break;
	    }
	}
    }

    tablePtr->numEntries--;
    if (typePtr->freeEntryProc) {
	typePtr->freeEntryProc (entryPtr);
    } else {
	ckfree((char *) entryPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteHashTable --
 *
 *	Free up everything associated with a hash table except for
 *	the record for the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is no longer usable.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteHashTable(tablePtr)
    register Tcl_HashTable *tablePtr;		/* Table to delete. */
{
    register Tcl_HashEntry *hPtr, *nextPtr;
    Tcl_HashKeyType *typePtr;
    int i;

    typePtr = tablePtr->typePtr;

    /*
     * Free up all the entries in the table.
     */

    for (i = 0; i < tablePtr->numBuckets; i++) {
	hPtr = tablePtr->buckets[i];
	while (hPtr != NULL) {
	    nextPtr = hPtr->nextPtr;
	    if (typePtr->freeEntryProc) {
		typePtr->freeEntryProc (hPtr);
	    } else {
		ckfree((char *) hPtr);
	    }
	    hPtr = nextPtr;
	}
    }

    /*
     * Free up the bucket array, if it was dynamically allocated.
     */

    if (tablePtr->buckets != tablePtr->staticBuckets) {
	ckfree((char *) tablePtr->buckets);
    }

    /*
     * Arrange for panics if the table is used again without
     * re-initialization.
     */

    tablePtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FirstHashEntry --
 *
 *	Locate the first entry in a hash table and set up a record
 *	that can be used to step through all the remaining entries
 *	of the table.
 *
 * Results:
 *	The return value is a pointer to the first entry in tablePtr,
 *	or NULL if tablePtr has no entries in it.  The memory at
 *	*searchPtr is initialized so that subsequent calls to
 *	Tcl_NextHashEntry will return all of the entries in the table,
 *	one at a time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_FirstHashEntry(tablePtr, searchPtr)
    Tcl_HashTable *tablePtr;		/* Table to search. */
    Tcl_HashSearch *searchPtr;		/* Place to store information about
					 * progress through the table. */
{
    searchPtr->tablePtr = tablePtr;
    searchPtr->nextIndex = 0;
    searchPtr->nextEntryPtr = NULL;
    return Tcl_NextHashEntry(searchPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NextHashEntry --
 *
 *	Once a hash table enumeration has been initiated by calling
 *	Tcl_FirstHashEntry, this procedure may be called to return
 *	successive elements of the table.
 *
 * Results:
 *	The return value is the next entry in the hash table being
 *	enumerated, or NULL if the end of the table is reached.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Tcl_NextHashEntry(searchPtr)
    register Tcl_HashSearch *searchPtr;	/* Place to store information about
					 * progress through the table.  Must
					 * have been initialized by calling
					 * Tcl_FirstHashEntry. */
{
    Tcl_HashEntry *hPtr;

    while (searchPtr->nextEntryPtr == NULL) {
	if (searchPtr->nextIndex >= searchPtr->tablePtr->numBuckets) {
	    return NULL;
	}
	searchPtr->nextEntryPtr =
		searchPtr->tablePtr->buckets[searchPtr->nextIndex];
	searchPtr->nextIndex++;
    }
    hPtr = searchPtr->nextEntryPtr;
    searchPtr->nextEntryPtr = hPtr->nextPtr;
    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * AllocStringEntry --
 *
 *	Allocate space for a Tcl_HashEntry containing the string key.
 *
 * Results:
 *	The return value is a pointer to the created entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_HashEntry *
AllocStringEntry(tablePtr, keyPtr)
    Tcl_HashTable *tablePtr;	/* Hash table. */
    VOID *keyPtr;		/* Key to store in the hash table entry. */
{
    CONST char *string = (CONST char *) keyPtr;
    Tcl_HashEntry *hPtr;
    unsigned int size;

    size = sizeof(Tcl_HashEntry) + strlen(string) + 1 - sizeof(hPtr->key);
    if (size < sizeof(Tcl_HashEntry))
	size = sizeof(Tcl_HashEntry);
    hPtr = (Tcl_HashEntry *) ckalloc(size);
    strcpy(hPtr->key.string, string);

    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * CompareStringKeys --
 *
 *	Compares two string keys.
 *
 * Results:
 *	The return value is 0 if they are different and 1 if they are
 *	the same.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CompareStringKeys(keyPtr, hPtr)
    VOID *keyPtr;		/* New key to compare. */
    Tcl_HashEntry *hPtr;		/* Existing key to compare. */
{
    register CONST char *p1 = (CONST char *) keyPtr;
    register CONST char *p2 = (CONST char *) hPtr->key.string;

    for (;; p1++, p2++) {
	if (*p1 != *p2) {
	    break;
	}
	if (*p1 == '\0') {
	    return 1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * HashStringKey --
 *
 *	Compute a one-word summary of a text string, which can be
 *	used to generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in
 *	string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
HashStringKey(tablePtr, keyPtr)
    Tcl_HashTable *tablePtr;	/* Hash table. */
    VOID *keyPtr;		/* Key from which to compute hash value. */
{
    register CONST char *string = (CONST char *) keyPtr;
    register unsigned int result;
    register int c;

    /*
     * I tried a zillion different hash functions and asked many other
     * people for advice.  Many people had their own favorite functions,
     * all different, but no-one had much idea why they were good ones.
     * I chose the one below (multiply by 9 and add new character)
     * because of the following reasons:
     *
     * 1. Multiplying by 10 is perfect for keys that are decimal strings,
     *    and multiplying by 9 is just about as good.
     * 2. Times-9 is (shift-left-3) plus (old).  This means that each
     *    character's bits hang around in the low-order bits of the
     *    hash value for ever, plus they spread fairly rapidly up to
     *    the high-order bits to fill out the hash value.  This seems
     *    works well both for decimal and non-decimal strings.
     */

    result = 0;
    while (1) {
	c = *string;
	if (c == 0) {
	    break;
	}
	result += (result<<3) + c;
	string++;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *	This procedure is invoked when the ratio of entries to hash
 *	buckets becomes too large.  It creates a new table with a
 *	larger bucket array and moves all of the entries into the
 *	new table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get re-hashed to new
 *	buckets.
 *
 *----------------------------------------------------------------------
 */

static void
RebuildTable(tablePtr)
    register Tcl_HashTable *tablePtr;	/* Table to enlarge. */
{
    int oldSize, count, index;
    Tcl_HashEntry **oldBuckets;
    register Tcl_HashEntry **oldChainPtr, **newChainPtr;
    register Tcl_HashEntry *hPtr;
    Tcl_HashKeyType *typePtr;

    oldSize = tablePtr->numBuckets;
    oldBuckets = tablePtr->buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */

    tablePtr->numBuckets *= 4;
    tablePtr->buckets = (Tcl_HashEntry **) ckalloc((unsigned)
	    (tablePtr->numBuckets * sizeof(Tcl_HashEntry *)));
    for (count = tablePtr->numBuckets, newChainPtr = tablePtr->buckets;
	    count > 0; count--, newChainPtr++) {
	*newChainPtr = NULL;
    }
    tablePtr->rebuildSize *= 4;
    tablePtr->downShift -= 2;
    tablePtr->mask = (tablePtr->mask << 2) + 3;

    typePtr = tablePtr->typePtr;

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    for (oldChainPtr = oldBuckets; oldSize > 0; oldSize--, oldChainPtr++) {
	for (hPtr = *oldChainPtr; hPtr != NULL; hPtr = *oldChainPtr) {
	    *oldChainPtr = hPtr->nextPtr;

	    if (typePtr->hashKeyProc == NULL
		|| typePtr->flags & TCL_HASH_KEY_RANDOMIZE_HASH) {
		index = RANDOM_INDEX (tablePtr, hPtr->hash);
	    } else {
		index = ((unsigned int) hPtr->hash) & tablePtr->mask;
	    }
	    hPtr->nextPtr = tablePtr->buckets[index];
	    tablePtr->buckets[index] = hPtr;
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != tablePtr->staticBuckets) {
	ckfree((char *) oldBuckets);
    }
}
