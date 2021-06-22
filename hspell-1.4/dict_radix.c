/* Copyright (C) 2003-2009 Nadav Har'El and Dan Kenigsberg */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

/* This is for declaring the uint32_t type, a type holding a 32-bit unsigned
   integer. It exists on Linux and on fairly modern Solaris, but
   maybe not anywhere else. We should use autoconf to solve this portability
   nightmare.
*/
#include <inttypes.h>

/* If the Zlib library is available, we use it for reading the compressed
   dictionaries, rather than opening a pipe to an external gzip process.
   In one measurement, this halved the loading time (0.1 sec to 0.05 sec).
   It also allowed Hspell to be used on systems where the Zlib library
   is available, but the gzip program is not (e.g., OpenOffice on MS-Windows).

   The definitions which are pretty bizarre, but help us convert the existing
   code into something that will work with zlib without ugly ifdefs
   everywhere (further ifdefs are only needed in some places). Note that
   when BUFFERED_ZLIB is enabled (and it is enabled by default here) we
   enable special buffered version of zlib (gzbuffered.h) instead of the
   normal zlib functions.
*/
#ifdef HAVE_ZLIB
#define BUFFERED_ZLIB
#undef FILE
#undef pclose
#undef getc
#ifdef BUFFERED_ZLIB
#include "gzbuffered.h"
#undef gzopen
#undef gzdopen
#define FILE void /* void* can be either normal FILE* or gzbFile*. Eek. */
#define gzopen(path,mode) gzb_open(path,mode)
#define gzdopen(path,mode) gzb_dopen(path,mode)
#define pclose(f) (gzb_close((gzbFile *)(f)))
#define getc(f) (gzb_getc(((gzbFile *)(f))))
#else
#include <zlib.h>
#define FILE void    /* FILE* is void*, a.k.a. voidp or gzFile */
#define pclose(f) (gzclose((f)))
#define getc(f) (gzgetc((f)))
#endif
#endif /* HAVE_ZLIB */

/* Our radix tree has four types of "nodes": leaf nodes, small nodes
 * (carrying up to SMALL_NODE_CHILDREN children), medium nodes (carrying up to
 * MEDIUM_NODE_CHILDREN) and full nodes carrying exactly NUM_LETTERS children.
 *
 * Since there are plenty of leaf nodes, we want these to be tiny, containing
 * basically just a value. Therefore we overload the same 32-bit "val_or_index"
 * position to be one of:
 * 1.  Empty  (in this case val_or_index==0)
 * 2.  Value  (value must be non-zero and 30 bit only!)
 * 3.  Index of full node (3 on highest 2 bits, the 30 lowest are the index)
 * 4.  Index of medium node (2 on highest 2 bits, the 30 lowest are the index)
 * 5.  Index of small node (1 on highest 2 bits, the 30 lowest are the index)
 */
#define CONST32(x) ((uint32_t)(x))
#define HIGHBITS ((CONST32(1)<<31) | (CONST32(1)<<30))
#define HIGHBITS_VALUE  (CONST32(0) << 30)
#define HIGHBITS_SMALL  (CONST32(1) << 30)
#define HIGHBITS_MEDIUM (CONST32(2) << 30)
#define HIGHBITS_FULL   (CONST32(3) << 30)
#define VALUEMASK (~HIGHBITS)

#define NUM_LETTERS 29  /* 27 Hebrew letters, " and ' */
/* When trying on the Hebrew dictionary, when there are only small and
 * full nodes, small_node_children=4 was the clear winner, taking 3363K
 * of memory.
 * When added medium nodes, there are two ties for minimal space usage
 * (at 2260K each): 2,8 and 3,8. Both have 1831 full nodes, 2,8 results in
 * 61771/25072 small/medium nodes, and 3,8 results in 71856/14987 small/medium
 * nodes.
 * One way to choose among them is to minimize search time. On average
 * searching a node with N children takes N/2 comparisons. If we pass
 * all nodes (and I doubt this is a meaningful measure... :( ) the 2,8
 * will make 162059 comparisons and 3,8 will make 167732. Again, roughly
 * the same, so I can't decide :(
 * Another deciding factor: read time. 2,8 is slightly quicker - I have
 * no idea why.
 *
 * Note: to minimize search time we might want to choose a set of sizes
 * which does not assure the smallest size. HOWEVER, one interesting thing
 * to note: the children in small and medium nodes are sorted. This might
 * mean that it is quicker to search the medium node using a binary search,
 * rather than linear? I don't know. Maybe for N=8, it ain't worth it.
 */
/*#define SMALL_NODE_CHILDREN 4*/
#define SMALL_NODE_CHILDREN 2
#define MEDIUM_NODE_CHILDREN 8

#if 0
 /*
 * NOTE:  SMALL-MEDIUM = 1-4 has an interesting advantage. At 2876K It wasn't
 * smallest (2-8 was, with 2257K) but it makes a lot of nodes full or
 * 1-child only (and therefore very quick to search) and only some nodes
 * with 4 children which is only slightly harder to search (only 2.5
 * comparisons needed on average).
 * */
/* search speed optimization */
#define SMALL_NODE_CHILDREN 1
#define MEDIUM_NODE_CHILDREN 4
#endif

struct node_index {
	/* if most-significant bit of val is on, it's an index. Otherwise,
	 * it's only a value (31 bit and nonzero).
	*/
	uint32_t val_or_index;
};

struct node {
	uint32_t value;
	struct node_index children[NUM_LETTERS];
};
struct node_small {
	uint32_t value;
	char chars[SMALL_NODE_CHILDREN];
	struct node_index children[SMALL_NODE_CHILDREN];
};
struct node_medium {
	uint32_t value;
	char chars[MEDIUM_NODE_CHILDREN];
	struct node_index children[MEDIUM_NODE_CHILDREN];
};


/* Note: char_to_letter prints a message when it comes across an invalid
   letter, so it should not be used in lookup(), only in reading the
   dictionary (which is assumed to contain only valid words). lookup()
   has its own implementation of this function inside it.
*/
static inline int char_to_letter(unsigned char c)
{
	if(c>=(unsigned char)'à' && c<(unsigned char)'à'+27){
		return c - (unsigned char)'à' + 2;
	} else if (c=='"'){
		return 0;
	} else if (c=='\''){
		return 1;
	} else {
		fprintf(stderr,"Hspell: unknown letter %c...\n",c);
		/* a silly thing to do, but what the heck */
		return 0;
	}
}

static inline unsigned char letter_to_char(int l)
{
	if(l>=2 && l<29){
		return l+(unsigned char)'à'-2;
	} else if(l==0){
		return '"';
	} else if(l==1){
		return '\'';
	} else {
		/* this will never happen in the current code: */
		fprintf(stderr,"Hspell: internal error: unknown letter %d... "
				"exiting.\n",l);
		exit(1);
	}
}

/* This routine was written for debugging purposes only, and not for
 * absolute efficiency.
 */
static void
do_print_tree(struct node *nodes, struct node_small *nodes_small,
	   struct node_medium *nodes_medium,
           struct node_index head, char *word, int len, int maxlen){
	int i;
	if(len>=maxlen){
		fprintf(stderr,"Hspell: do_print_tree(): warning: buffer overflow.\n");
		return;
	}
	if((head.val_or_index & HIGHBITS) == HIGHBITS_FULL){
		struct node *n = &nodes[head.val_or_index & VALUEMASK];
		if(n->value){
			word[len]='\0';
			printf("%s %d\n", word, n->value);
		}
		for(i=0;i<NUM_LETTERS;i++){
			word[len]=letter_to_char(i);
			do_print_tree(nodes,nodes_small,nodes_medium,
					n->children[i],word,len+1,maxlen);
		}
	} else if((head.val_or_index & HIGHBITS) == HIGHBITS_SMALL){
		struct node_small *n = &nodes_small[head.val_or_index & VALUEMASK];
		if(n->value){
			word[len]='\0';
			printf("%s %d\n", word, n->value);
		}
		for(i=0;i<SMALL_NODE_CHILDREN;i++){
			if(n->chars[i]){
				word[len]=n->chars[i];
				do_print_tree(nodes,nodes_small,nodes_medium,
					n->children[i],word,len+1,maxlen);
			}
		}
	} else if((head.val_or_index & HIGHBITS) == HIGHBITS_MEDIUM){
		struct node_medium *n = &nodes_medium[head.val_or_index & VALUEMASK];
		if(n->value){
			word[len]='\0';
			printf("%s %d\n", word, n->value);
		}
		for(i=0;i<MEDIUM_NODE_CHILDREN;i++){
			if(n->chars[i]){
				word[len]=n->chars[i];
				do_print_tree(nodes,nodes_small,nodes_medium,
					n->children[i],word,len+1,maxlen);
			}
		}
	} else if(head.val_or_index){
		word[len]='\0';
		printf("%s %d\n", word, head.val_or_index);
	}
}

struct dict_radix {
	/* The nodes used by the radix tree representation of the dictionary */
	int nnodes_small, size_nodes_small;
	struct node_small *nodes_small;

	int nnodes_medium, size_nodes_medium;
	struct node_medium *nodes_medium;

	int nnodes, size_nodes;
	struct node *nodes;

	struct node_index head;

	/* Freelist of recycled small nodes. As more words are added to the
	   dictionary in the process of read_dict(), small nodes become
	   medium and medium nodes become full. Because these small/medium
	   nodes that are no longer needed are in the middle of the node
	   list, we keep them aside in a freelist. They are recycled quickly,
	   as new small/medium nodes are continued to be created.
	 */
	int free_nodes_small[16], nfree_nodes_small;
	int free_nodes_medium[16], nfree_nodes_medium;

	int nwords;
};

/* new_dict_radix is the constructor for an opaque (to the includer of
   dict_radix.h) object.
*/
struct dict_radix *
new_dict_radix(void)
{
	struct dict_radix *dict;
	dict= (struct dict_radix *) malloc(sizeof(struct dict_radix));
	/* By default, zero all fields in dict_radix */
	if(dict)
		memset(dict, 0, sizeof(*dict));
	return dict;
}

/* Note that delete_dict_radix frees everything inside a dict_radix, and
   the dict_radix structure itself. The pointer given to it is no longer
   a valid pointer after this call.
*/
void
delete_dict_radix(struct dict_radix *dict)
{
	if(!dict)
		return; /* allow deleting null object, like in C++... */
	if(dict->nodes_small)
		free(dict->nodes_small);
	if(dict->nodes_medium)
		free(dict->nodes_medium);
	if(dict->nodes)
		free(dict->nodes);
	free(dict);
}

int
allocate_nodes(struct dict_radix *dict, int nsmall, int nmedium, int nfull)
{
	/* if already allocated, it's an error */
	if(dict->nodes)
		return -1;

	dict->nodes_small = malloc(sizeof(struct node_small)*nsmall);
	dict->size_nodes_small = nsmall;

	dict->nodes_medium = malloc(sizeof(struct node_medium)*nmedium);
	dict->size_nodes_medium = nmedium;

	dict->nodes = malloc(sizeof(struct node)*nfull);
	dict->size_nodes = nfull;

	if(dict->nodes_small==NULL || dict->nodes_medium==NULL ||
	   dict->nodes==NULL)
		return -2;

	return 0;
}


/* Efficiently read a compressed dictionary from the given directory.
   Use memory pre-allocation hints from another file in this directory.

   returns 1 on success, 0 on failure.

   TODO: there are too many printouts here. We need to return error
   numbers instead of all those printouts.
*/

#define PREFIX_FILE

#ifdef PREFIX_FILE
static int do_read_dict(FILE *fp, FILE *prefixes, struct dict_radix *dict);
#else
static int do_read_dict(FILE *fp, struct dict_radix *dict);
#endif

int
read_dict(struct dict_radix *dict, const char *dir)
{
	if(dir){
		FILE *fp;
		char s[1024];
		int small,medium,full,ret;
#ifdef PREFIX_FILE
		FILE *prefixes;
#endif

		snprintf(s,sizeof(s),"%s.sizes",dir);
		if(!(fp=fopen(s,"r"))){
			fprintf(stderr,"Hspell: can't open %s.\n",s);
			return 0;
		}
		if(fscanf(fp,"%d %d %d",&small,&medium,&full)!=3){
			fprintf(stderr,"Hspell: can't read from %s.\n",s);
			return 0;
		}
		fclose(fp);

#ifdef HAVE_ZLIB
		if(!(fp=gzopen(dir,"r"))){
			fprintf(stderr,"Hspell: can't open %s.\n",dir);
			return 0;
		}
#else
		snprintf(s,sizeof(s),"gzip -dc '%s'",dir);
		if(!(fp=popen(s,"r"))){
			fprintf(stderr,"Hspell: can't run %s.\n",s);
			return 0;
		}
#endif /* HAVE_ZLIB */

#ifdef PREFIX_FILE
#ifdef HAVE_ZLIB
		snprintf(s,sizeof(s),"%s.prefixes",dir);
		if(!(prefixes=gzopen(s,"rb"))){
			fprintf(stderr,"Hspell: can't open %s.\n",s);
			return 0;
		}
#else
		snprintf(s,sizeof(s),"gzip -dc '%s.prefixes'",dir);
		if(!(prefixes=popen(s,"rb"))){
			fprintf(stderr,"Hspell: can't run %s.\n",s);
			return 0;
		}
#endif /* HAVE_ZLIB */
#endif

		allocate_nodes(dict,small,medium,full);
#ifdef PREFIX_FILE
		ret=do_read_dict(fp, prefixes, dict);
		pclose(prefixes);
#else
		ret=do_read_dict(fp, dict);
#endif
		pclose(fp);
		return ret;
	} else {
#ifdef HAVE_ZLIB
		/* note that gzopen also works on non-gzipped files */
		FILE *in=gzdopen(fileno(stdin),"r");
#ifdef PREFIX_FILE
		FILE *zero=gzopen("/dev/zero","r");
#endif
#else
		FILE *in=stdin;
#ifdef PREFIX_FILE
		FILE *zero=fopen("/dev/zero","r");
#endif
#endif /* HAVE_ZLIB */

#ifdef PREFIX_FILE
		return do_read_dict(in, zero, dict);
#else
		return do_read_dict(in, dict);
#endif
	}
}

#ifdef PREFIX_FILE
static int do_read_dict(FILE *fp, FILE *prefixes, struct dict_radix *dict)
#else
static int do_read_dict(FILE *fp, struct dict_radix *dict)
#endif
{
	struct node_index *stack[256];
	int sdepth=0;
	int c,n,cc;
	/* Local copies of dict-> variables, for efficiency. */
	int nwords=0;
	struct node *nodes = dict->nodes;
	struct node_small *nodes_small = dict->nodes_small;
	struct node_medium *nodes_medium = dict->nodes_medium;
	int nnodes_small=0, nnodes_medium=0, nnodes=0;

	if(dict->nnodes||dict->nnodes_small||dict->nnodes_medium||
	   dict->nwords){
		fprintf(stderr, "Hspell: do_read_dict(): called for a non-"
			"empty dictionary\n");
		return 0;
	}
	if(!nodes||!nodes_small||!nodes_medium){
		fprintf(stderr, "Hspell: do_read_dict(): allocate_nodes() must"
			" be called first\n");
		return 0;
	}

	memset(&nodes[nnodes], 0, sizeof(nodes[nnodes]));
	dict->head.val_or_index=(nnodes++) | HIGHBITS_FULL;
	stack[0]=&dict->head;
	sdepth=0;
	while((c=getc(fp))!=EOF){
		if(c>='0' && c<='9'){
			/* new word - finalize old word first (set value) */
			nwords++; /* statistics */
			/* assert(!stack[sdepth]->val_or_index) */
#ifdef PREFIX_FILE
			stack[sdepth]->val_or_index=getc(prefixes);
#else
			stack[sdepth]->val_or_index=nwords; /** TODO: different values */
#endif
			/* and read how much to go back */
			n=0;
			do {
				/* base 10... */
				n*=10;
				n+=(c-'0');
			} while ((c=getc(fp))!=EOF && c>='0' && c<='9');
			sdepth-=n;
			if(sdepth<0 || sdepth >= (sizeof(stack)/sizeof(stack[0]))-1){
				fprintf(stderr,"Hspell: bad backlength %d... giving up\n", sdepth);
				return 0;
			}
			/* we got a new letter c - continue the loop */
		}
		/* word letter - add it */
		if(sdepth>=sizeof(stack)/sizeof(stack[0])-1){
			fprintf(stderr,"Hspell: word too long... giving up\n");
			return 0;
		}
		cc=char_to_letter(c);
		/* make sure previous node is a small or full node, not just a
		 * value, and if it is small, that it's not full */
		if((stack[sdepth]->val_or_index & HIGHBITS)==HIGHBITS_VALUE){
			int chosen;
			if(dict->nfree_nodes_small){
				chosen=dict->free_nodes_small
					[--(dict->nfree_nodes_small)];
			} else {
				chosen=nnodes_small;
				if(nnodes_small>=dict->size_nodes_small){
					fprintf(stderr,"Hspell: Realloc needed (small) - failing.\n");
					return 0;
				}
				nnodes_small++;
			}
			memset(&nodes_small[chosen], 0, sizeof(nodes_small[chosen]));
			nodes_small[chosen].value = stack[sdepth]->val_or_index;
			stack[sdepth]->val_or_index = chosen | HIGHBITS_SMALL;

			nodes_small[chosen].chars[0]=c;
			stack[sdepth+1] = &nodes_small[chosen].children[0];
		} else if((stack[sdepth]->val_or_index & HIGHBITS)==HIGHBITS_SMALL){
			int j;
			struct node_small *n=
			   &nodes_small[stack[sdepth]->val_or_index&VALUEMASK];
			/* is the small node not full yet? */
			for(j=0;j<SMALL_NODE_CHILDREN;j++)
				if(!n->chars[j]){
					n->chars[j]=c;
					stack[sdepth+1] = &n->children[j];
					break;
				}
			if(j==SMALL_NODE_CHILDREN){
				/* small node full! convert it to medium node */
				int chosen;
				if(dict->nfree_nodes_medium){
					chosen=dict->free_nodes_medium
						[--(dict->nfree_nodes_medium)];
				} else {
					chosen=nnodes_medium;
					if(nnodes_medium>=dict->size_nodes_medium){
						fprintf(stderr,"Hspell: Realloc needed (medium) - failing.\n");
						return 0;
					}
					nnodes_medium++;
				}
				memset(&nodes_medium[chosen], 0, sizeof(nodes_medium[chosen]));
				if(dict->nfree_nodes_small>=
				   sizeof(dict->free_nodes_small)/
				   sizeof(dict->free_nodes_small[0])){
					fprintf(stderr,"Hspell: overflow in free_nodes_small.\n");
					return 0;
				}
				dict->free_nodes_small
					[(dict->nfree_nodes_small)++]=
					stack[sdepth]->val_or_index & VALUEMASK;
				stack[sdepth]->val_or_index = chosen | HIGHBITS_MEDIUM;
				/* copy the children from n to nodes[nnodes]: */
				/* TODO: use memcpy instead! */
				nodes_medium[chosen].value = n->value;
				for(j=0;j<SMALL_NODE_CHILDREN;j++){
					nodes_medium[chosen].chars[j]=
						n->chars[j];
					nodes_medium[chosen].children[j]=
						n->children[j];
				}
				/* and finally choose the next child */
				nodes_medium[chosen].chars[SMALL_NODE_CHILDREN]=
					c;
				stack[sdepth+1] = &nodes_medium[chosen].
					children[SMALL_NODE_CHILDREN];
			}
		} else if((stack[sdepth]->val_or_index & HIGHBITS)==HIGHBITS_MEDIUM){
			int j;
			struct node_medium *n=
			   &nodes_medium[stack[sdepth]->val_or_index&VALUEMASK];
			/* is the medium node not full yet? */
			for(j=0;j<MEDIUM_NODE_CHILDREN;j++)
				if(!n->chars[j]){
					n->chars[j]=c;
					stack[sdepth+1] = &n->children[j];
					break;
				}
			if(j==MEDIUM_NODE_CHILDREN){
				/* medium node full! convert it to full node */
				if(nnodes>=dict->size_nodes){
					fprintf(stderr,"Hspell: Realloc needed (full) - failing.\n");
					return 0;
				}
				memset(&nodes[nnodes], 0, sizeof(nodes[nnodes]));
				nodes[nnodes].value = n->value;
				if(dict->nfree_nodes_medium>=
				   sizeof(dict->free_nodes_medium)/
				   sizeof(dict->free_nodes_medium[0])){
					fprintf(stderr,"Hspell: overflow in free_nodes_medium.\n");
					return 0;
				}
				dict->free_nodes_medium
					[(dict->nfree_nodes_medium)++]=
					stack[sdepth]->val_or_index & VALUEMASK;
				stack[sdepth]->val_or_index = nnodes | HIGHBITS_FULL;
				/* copy the children from n to nodes[nnodes]: */
				for(j=0;j<MEDIUM_NODE_CHILDREN;j++)
					nodes[nnodes].children[char_to_letter(
						n->chars[j])]=
						n->children[j];
				/* and finally choose the next child */
				stack[sdepth+1] = &nodes[nnodes].children[cc];
				nnodes++;
			}
		} else { /* HIGHBITS_FULL */
			stack[sdepth+1] = &nodes[
			  stack[sdepth]->val_or_index & VALUEMASK].children[cc];
		}
		sdepth++;
	}
	/* output last word */
	nwords++; /* statistics */
#ifdef PREFIX_FILE
	stack[sdepth]->val_or_index=getc(prefixes);
#else
	stack[sdepth]->val_or_index=nwords; /** TODO: different values */
#endif

	/* return local copies to dict-> structure */
	dict->nwords=nwords;
	dict->nnodes_small=nnodes_small;
	dict->nnodes_medium=nnodes_medium;
	dict->nnodes=nnodes;

	return 1;
}

void
print_stats(struct dict_radix *dict)
{
	fprintf(stderr,	"%d words in %d full nodes, %d medium nodes, "
		"%d small nodes.\n", dict->nwords, dict->nnodes,
		dict->nnodes_medium, dict->nnodes_small);
	fprintf(stderr, "%d nfree_nodes_small %d nfree_nodes_medium.\n",
		dict->nfree_nodes_small,dict->nfree_nodes_medium);
	fprintf(stderr, "node memory filled: %d K\n",
	       (int)(dict->nnodes*sizeof(struct node)
		+ dict->nnodes_small*sizeof(struct node_small)
		+ dict->nnodes_medium*sizeof(struct node_medium)
		       )/1024);
}

void
print_tree(struct dict_radix *dict)
{
	char word[256];
	do_print_tree(dict->nodes,dict->nodes_small,dict->nodes_medium,
		      dict->head,word,0,sizeof(word));

}

void
print_sizes(struct dict_radix *dict)
{
	printf("%d %d %d\n", dict->nnodes_small, dict->nnodes_medium,
		dict->nnodes);
}

int
lookup(const struct dict_radix *dict, const char *word)
{
	struct node_index current = dict->head;
	for(;;){
		switch(current.val_or_index & HIGHBITS){
		case HIGHBITS_VALUE:
			if(*word){
				/* The word isn't over yet but we reached a
				   leaf node. So the word isn't in the dict */
				return 0;
			} else {
				return current.val_or_index & VALUEMASK;
			}
			break;
		case HIGHBITS_SMALL:
			if(*word){
				struct node_small *n =
				      &dict->nodes_small[current.val_or_index
							 & VALUEMASK];
#if SMALL_NODE_CHILDREN==2
				if(n->chars[0]==*word)
					current=n->children[0];
				else if(n->chars[1]==*word)
					current=n->children[1];
				else
					return 0; /* not found... */
#else
#error "small node lookup not implemented except for 2 children."
#endif
			} else {
				return dict->nodes_small[current.val_or_index
							 & VALUEMASK]
					.value;
			}
			break;
		case HIGHBITS_MEDIUM:
			if(*word){
				struct node_medium *n =
				      &dict->nodes_medium[current.val_or_index
							  & VALUEMASK];
#if MEDIUM_NODE_CHILDREN==8
				register char c=*word, *cs=n->chars;
				/* TODO: use binary search? stop searching
				   on the first 0? All these optimizations
				   are probably useless for 8 chars... */
				if(*(cs++)==c)      current=n->children[0];
				else if(*(cs++)==c) current=n->children[1];
				else if(*(cs++)==c) current=n->children[2];
				else if(*(cs++)==c) current=n->children[3];
				else if(*(cs++)==c) current=n->children[4];
				else if(*(cs++)==c) current=n->children[5];
				else if(*(cs++)==c) current=n->children[6];
				else if(*(cs++)==c) current=n->children[7];
				else
					return 0; /* not found... */
#else
#error "medium node lookup not implemented except for 8 children."
#endif
			} else {
				return dict->nodes_medium[current.val_or_index
							  & VALUEMASK]
					.value;
			}
			break;
		case HIGHBITS_FULL:
			if(*word){
				/* the following is a copy of char_to_letter */
				register int ind;
				register unsigned char c = *word;
				if(c>=(unsigned char)'à' &&
				   c<(unsigned char)'à'+27)
					ind = c - (unsigned char)'à' + 2;
				else if (c=='"')
					ind = 0;
				else if (c=='\'')
					ind = 1;
				else
					return 0; /* non-Hebrew letter */
				current=dict->nodes[current.val_or_index
						    & VALUEMASK]
					.children[ind];
			} else {
				return dict->nodes[current.val_or_index
						   & VALUEMASK].value;
			}
			break;
		}
		word++;
	}
}
