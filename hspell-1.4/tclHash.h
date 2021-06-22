/* from generic/tcl.h: */

/* TCL_HASH_KEY_RANDOMIZE_HASH:
 *				There are some things, pointers for example
 *				which don't hash well because they do not use
 *				the lower bits. If this flag is set then the
 *				hash table will attempt to rectify this by
 *				randomising the bits and then using the upper
 *				N bits as the index into the table.
 */
#define TCL_HASH_KEY_RANDOMIZE_HASH 0x1

#define TCL_STRING_KEYS		0

#include <stdio.h>

#   define _ANSI_ARGS_(x)	x
#define VOID void
#define CONST const
#define EXTERN extern
typedef void *ClientData;
struct Tcl_Obj;
typedef struct Tcl_Obj Tcl_Obj;

/*
 * Forward declarations of Tcl_HashTable and related types.
 */
typedef struct Tcl_HashKeyType Tcl_HashKeyType;
typedef struct Tcl_HashTable Tcl_HashTable;
typedef struct Tcl_HashEntry Tcl_HashEntry;
typedef struct Tcl_HashSearch Tcl_HashSearch;

typedef unsigned int (Tcl_HashKeyProc) _ANSI_ARGS_((Tcl_HashTable *tablePtr,
        VOID *keyPtr));
typedef int (Tcl_CompareHashKeysProc) _ANSI_ARGS_((VOID *keyPtr,
        Tcl_HashEntry *hPtr));
typedef Tcl_HashEntry *(Tcl_AllocHashEntryProc) _ANSI_ARGS_((
        Tcl_HashTable *tablePtr, VOID *keyPtr));
typedef void (Tcl_FreeHashEntryProc) _ANSI_ARGS_((Tcl_HashEntry *hPtr));

/*
 * Structure definition for an entry in a hash table.  No-one outside
 * Tcl should access any of these fields directly;  use the macros
 * defined below.
 */

struct Tcl_HashEntry {
    Tcl_HashEntry *nextPtr;             /* Pointer to next entry in this
                                         * hash bucket, or NULL for end of
                                         * chain. */
    Tcl_HashTable *tablePtr;            /* Pointer to table containing entry. */
    unsigned int hash;                  /* Hash value. */
    ClientData clientData;              /* Application stores something here
                                         * with Tcl_SetHashValue. */
    union {                             /* Key has one of these forms: */
        char *oneWordValue;             /* One-word value for key. */
        Tcl_Obj *objPtr;                /* Tcl_Obj * key value. */
        int words[1];                   /* Multiple integer words for key.
                                         * The actual size will be as large
                                         * as necessary for this table's
                                         * keys. */
        char string[4];                 /* String for key.  The actual size
                                         * will be as large as needed to hold
                                         * the key. */
    } key;                              /* MUST BE LAST FIELD IN RECORD!! */
};

#define TCL_STRING_KEYS         0

/*
 * Macros for clients to use to access fields of hash entries:
 */

#define Tcl_GetHashValue(h) ((h)->clientData)
#define Tcl_SetHashValue(h, value) ((h)->clientData = (ClientData) (value))
/*#define Tcl_GetHashKey(tablePtr, h) \
  ((char *) ((h)->key.string))*/
#define Tcl_GetHashKey(tablePtr, h) ((h)->key.string)

EXTERN Tcl_HashEntry *	Tcl_NextHashEntry _ANSI_ARGS_((
				Tcl_HashSearch * searchPtr));
EXTERN Tcl_HashEntry *	Tcl_FirstHashEntry _ANSI_ARGS_((Tcl_HashTable *tablePtr,
				Tcl_HashSearch * searchPtr));
EXTERN void Tcl_InitHashTable _ANSI_ARGS_((Tcl_HashTable *tablePtr, int keyType));
EXTERN void Tcl_DeleteHashTable _ANSI_ARGS_((Tcl_HashTable *tablePtr));
EXTERN Tcl_HashEntry *Tcl_CreateHashEntry _ANSI_ARGS_((Tcl_HashTable *tablePtr, CONST char *key, int *newPtr));
EXTERN Tcl_HashEntry *Tcl_FindHashEntry _ANSI_ARGS_((Tcl_HashTable *tablePtr, CONST char *key));

/*
 * Structure definition for a hash table.  Must be in tcl.h so clients
 * can allocate space for these structures, but clients should never
 * access any fields in this structure.
 */

#define TCL_SMALL_HASH_TABLE 4
struct Tcl_HashTable {
    Tcl_HashEntry **buckets;            /* Pointer to bucket array.  Each
                                         * element points to first entry in
                                         * bucket's hash chain, or NULL. */
    Tcl_HashEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
                                        /* Bucket array used for small tables
                                         * (to avoid mallocs and frees). */
    int numBuckets;                     /* Total number of buckets allocated
                                         * at **bucketPtr. */
    int numEntries;                     /* Total number of entries present
                                         * in table. */
    int rebuildSize;                    /* Enlarge table when numEntries gets
                                         * to be this large. */
    int downShift;                      /* Shift count used in hashing
                                         * function.  Designed to use high-
                                         * order bits of randomized keys. */
    int mask;                           /* Mask value used in hashing
                                         * function. */
    int keyType;                        /* Type of keys used in this table. 
                                         * It's either TCL_CUSTOM_KEYS,
                                         * TCL_STRING_KEYS, TCL_ONE_WORD_KEYS,
                                         * or an integer giving the number of
                                         * ints that is the size of the key.
                                         */
    Tcl_HashKeyType *typePtr;           /* Type of the keys used in the
                                         * Tcl_HashTable. */
};

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:
 */
struct Tcl_HashSearch {
    Tcl_HashTable *tablePtr;            /* Table being searched. */
    int nextIndex;                      /* Index of next bucket to be
                                         * enumerated after present one. */
    Tcl_HashEntry *nextEntryPtr;        /* Next entry to be enumerated in the
                                         * the current bucket. */
};
