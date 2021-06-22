/* Copyright 2005-2009 Nadav Har'El and Dan Kenigsberg */

/* specfilter.c - word prefix-specifier 
 * The way prefixes currently work in Hspell is that each word has an 8-bit
 * (only 6 are used) "specifier", and each prefix has a bit mask, and the
 * word+prefix combination is accepted if one of the bits is 1 in both the
 * word specifier and prefix mask. 
 * The idea in this is that each bit corresponds to a prefix feature that
 * words need, and certain prefixes supply. If a word has two or more meanings,
 * the word's specifier might get two or more 1 bits.
 *
 * Now, it turns out (see bug 74) that while the word specifiers can take
 * on many values (currently, 32), some specifiers are actually equivalent,
 * in the sense that they end up allowing or disallowing exactly the same
 * set of prefixes. For example, a word with specifier 2 (PS_L) and a word
 * with specifier 3 (PS_L | PS_B) accepts the same prefixes because in our
 * existing prefix set (sett genprefixes.pl), every prefix which supplies
 * PS_B also supplies PS_L. Another example, a word with specifier  24
 * (PS_IMPER | PS_NONDEF) can get the same prefixes as a word with just a
 * specifier of 8 (PS_NONDEF).
 *
 * The goal of this program is to find which sets of word specifiers are
 * equivalent in the above sense, and given an input stream of word
 * specifiers (e.g., uncompressed hebrew.wgz.prefixes) it replaces all
 * different specifiers in one equivalence class to the same member
 * (the list of values left after this process is known in Mathematics
 * as a quotient set).
 * 
 * The purpose of all this is to reduce the number of different word
 * specifiers present in hebrew.wgz.prefixes. For example, in Hspell 0.9
 * this brought the number from 32 down to 9. This allows slightly (10%)
 * better compression of hebrew.wgz.prefixes, but more importantly,
 * allows us to generate a much smaller affix file for aspell (see
 * mk_he_affix.c), because it will contain just 9 prefix sets instead of 32.
 *
 * CAVEAT:
 *
 * I'm still not sure whether it is wise or not to use this process on
 * the final hebrew.wgz.prefixes used by Hspell. One one hand it will make
 * it slightly smaller, but on the other hand it makes use of information
 * previously available only in the code (prefixes.c - the list of prefixes
 * and their masks) inside the word list. Meaning that if the prefix list
 * code changes, the word data will need to be changed as well - a situation
 * we didn't have previously.
 * Moreover, it will also mean that we will not be able to have run-time
 * options that chose among different prefix sets. Luckily, the "-h" option
 * is fine in this respect (see comment below on why), but it is conceivable
 * (but not likely) that in the future we might want to use completely
 * different sets of prefix that behave differently. (?? what actually
 * matters is just the bag of different masks in the masks[] array))
 */

#include "prefixes.c"

#include <stdlib.h>

/* NOTE: currently, the equivalence of two word specifiers does not depend
   on whether He Hashe'ela is allowed or not. This is because He Hashe'ela
   only adds another prefix like shin hashimush - so whatever specifiers
   that can be distinguished by He Hashe'ela can already be distinguished
   by the shin.
   It is important to remember that this fact may not remain true if we add
   more options of prefix bitmasks arrays, so this decision might need to
   be revisited.
*/
#define MASKS masks_noH
#define SPECBITS 6 /* maximum number of bits in PrefixBits.pl */

#define NMASKS (sizeof(MASKS)/sizeof(MASKS[0])-1)
#define NSPECS (1<<SPECBITS)

/* filled by genequiv: */
int masks[NSPECS], nmasks;
int equivalent[NSPECS];

/* Check if two word specs, mask1 and mask2, are equivalent with all
   known prefixes, in the sense that each prefix is either allowed or
   disallowed the same for both masks.
*/
static int checkequiv(int spec1, int spec2)
{
	int k;
	for(k=0; k<nmasks; k++)
		if( ((masks[k]&spec1) != 0) != ((masks[k]&spec2) != 0) )
			return 0;
	return 1;
}

/* Fill the equivalent[] array, giving to each specifier an equivalent
 * specifier chosen as the representative of its equivalence class.
 *. As a by product, also fill masks[] array (different masks).
 */
static void genequiv(void)
{
	int i,j;
        /* Stage 1: prepare a smaller masks[] array from the MASKS[] array.
	   (this is just a silly optimization - we could have used the MASKS
	   array directly just fine).

	   In checkequiv() we need to check all the prefix's masks. But doing
	   the loop directly over all masks is quite wasteful, because prefixes
	   are generated in groups and many of them have the same masks. For
	   example, Hspell 0,9's masks_noH has 242 entries, but just five
	   distinct values: 60 (two prefixes - namely waw and the null prefix),
	   43 (4 prefixes), 44 (14 prefixes), 32 (48 prefixes) and
	   42 (174 prefixes). So we copy the unique masks in MASKS into
	   the masks[] array. This makes finding the equivalent[] array 5
	   times quicker on my machine (just 1/40,000 of a second!)
	   
	   TODO: can this knowledge be somehow be used to squeeze he_affix.dat
	   even further, because this means that each of the 9 prefix sets
	   mentioned above are all just combinations of these 5 disjoint
	   (and therefore smaller) sets?
	*/
	for(i=0;i<NSPECS;i++) masks[i]=0;
	for(i=0;i<NMASKS;i++)
		masks[MASKS[i]]++;/* TODO: check that 0<=MASKS[i]<NSPECS */
	nmasks=0;
	for(i=0;i<NSPECS;i++) /* bring all nonzero to front */
		if(masks[i])
			masks[nmasks++] = i;

	/* Stage 2: find the equivalent[] array */
	for(i=0;i<NSPECS;i++) equivalent[i]=0;

	for(i=0;i<NSPECS;i++){
		if(equivalent[i]) /*already in equivalence class*/
			continue;
		equivalent[i]=i; /*new equivalence class, find more members:*/
		for(j=i+1; j<NSPECS; j++){
			if(equivalent[j])
				continue;
			if(checkequiv(i, j))
				equivalent[j]=i;
		}
	}
}

#include <stdio.h>
int main(void){
	int i,j;
	int num;

	genequiv();

#if 0
	fprintf(stderr, "Prefix specifier equivalences:\n");
	for(i=0;i<NSPECS;i++){
		fprintf(stderr, "%d: %d\n",i,equivalent[i]);
	}
	fprintf(stderr, "\n");
#endif

	fprintf(stderr, "%d unique prefix masks:", nmasks);
	for(i=0;i<nmasks;i++){
		fprintf(stderr, " %d", masks[i]);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Prefix specifier equivalence classes:\n");
	num=0;
	for(i=0;i<NSPECS;i++){
		if(equivalent[i]<i) /* printed this class before... */
			continue;
		fprintf(stderr,"%d: %d",num++,i);
		for(j=i+1; j<NSPECS; j++){
			if(equivalent[j]==i){
				fprintf(stderr," %d",j);
			}
		}
		fprintf(stderr,"\n");
	}

	/* Read hebrew.wgz.prefixes, and replace each specifier by the
	   canonic member of its equivalence class */
	while ((i=getchar())!= EOF) {
		if(i >= NSPECS){
			fprintf(stderr, "value %d in hebrew.wgz.prefixes out of bound\n", i);
			exit(1);
		}
		putchar(equivalent[i]);
	}
	return 0;
}
