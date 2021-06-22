/* Copyright (C) 2003 Nadav Har'El and Dan Kenigsberg */

#include <stdio.h>
#include <string.h>

#include "hspell.h"

extern int hspell_debug;

/* functions for checking valid gimatria */
static unsigned int
gim2int(const char *w){
	int n=0;
	if(hspell_debug) fprintf(stderr,"gim2int got %s ",w);
	while(*w){
		switch(*w){
		case '\'':
			/* ad-hoc change: ג' can mean with 3 or 3000. Our
			 * check that the ' is not in the end forces to be 3,
			 * because I don't want to recognize stuff like תריג'
			 * = 613,000.
			 * TODO: consider if I should remove this if(w[1])
			 * line.
			 * */
			if(w[1])
			n*=1000;
			break;
		case 'א': n+=1; break;
		case 'ב': n+=2; break;
		case 'ג': n+=3; break;
		case 'ד': n+=4; break;
		case 'ה': n+=5; break;
		case 'ו': n+=6; break;
		case 'ז': n+=7; break;
		case 'ח': n+=8; break;
		case 'ט': n+=9; break;
		case 'י': n+=10; break;
		case 'כ': case 'ך': n+=20; break;
		case 'ל': n+=30; break;
		case 'מ': case 'ם': n+=40; break;
		case 'נ': case 'ן': n+=50; break;
		case 'ס': n+=60; break;
		case 'ע': n+=70; break;
		case 'פ': case 'ף': n+=80; break;
		case 'צ': case 'ץ': n+=90; break;
		case 'ק': n+=100; break;
		case 'ר': n+=200; break;
		case 'ש': n+=300; break;
		case 'ת': n+=400; break;
		/* ignore " characters */
		}
		w++;
	}
	if(hspell_debug) fprintf(stderr,"returning %d\n",n);
	return n;
}
#if 0
void
int2gim(int n, char *buf, int sizebuf)
{
	int i;
	int nn, divisor;
	if(n<=0){
		/* no gimatria... */
		if(sizebuf) buf[0]='\0';
		return;
	}
	if(n>=1000*1000*1000) divisor=1000*1000*1000;
	else if(n>=1000*1000) divisor=1000*1000;
	else if(n>=1000) divisor=1000;
	else divisor=1;

#define out1(c) {if(i<sizebuf-1){ buf[i++]=(c); }}

	while(divisor){
		nn=n/divisor;
#define check(
		DO HERE THE NORMAL CODE FOR nn
		n-=nn*divisor;
		divisor/=1000;
		if(divisor)
			out1('\'');
	}
	buf[i]='\0';
}
#endif
/* print Hebrew numerals. The output string must be big enough to store
   the resulting hebrew number (30 characters is more then enough)!
*/
/* appendStr appends the src string at the given dst pointer, and return
   a pointer to the end of the resulting string (after the original dst
   string). Note that a null is appended to the resulting string, and the
   returned pointer is actually a pointer to it.
*/
static char *
appendStr(src,dst)
        char *src,*dst;
{
        while(*src){
                *(dst++)=*(src++);
        }
        *dst='\0';
        return dst;
}
static void
int2gim(unsigned int n, char *buf)
{
	static char *digits[3][9] = {
		{"א","ב","ג","ד","ה","ו","ז","ח","ט"},
		{"י","כ","ל","מ","נ","ס","ע","פ","צ"},
		{"ק","ר","ש","ת","קת","רת","שת","תת","קתת"}
        };
        static char *special[2] = {"וט","זט"};
        int i = 0;
	char *b=buf, *bleft, *bright;
	*b='\0';

	if(hspell_debug) fprintf(stderr,"int2gim got %d ",n);
        while (n>0) {
                if (i == 3) {i = 0; b=appendStr("\'", b);}
                if (!i && (n%100 == 15 || n%100 == 16)) {
                        b=appendStr(special[n%100 - 15],b);
                        n /= 100;
                        i = 2;
                } else {
                        if (n%10) b=appendStr(digits[i][n%10 - 1],
                                                b);
                        n /= 10;
                        i++;
                }
        }
	/* reverse the string */
	if(hspell_debug) fprintf(stderr,"before %s\n",buf);
	if(buf[0]!='\0')
	for(bleft=buf, bright=b-1; bright>bleft; bleft++, bright--){
		char tmp;
		tmp=*bleft;
		*bleft=*bright;
		*bright=tmp;
	}
	if(hspell_debug) fprintf(stderr,"after %s\n",buf);
	/* we decided gimatria to end in final letters */
	if(buf[0]){
		switch(b[-1]){
		case 'כ': b[-1]='ך'; break;
		case 'מ': b[-1]='ם'; break;
		case 'נ': b[-1]='ן'; break;
		case 'צ': b[-1]='ץ'; break;
		case 'פ': b[-1]='ף'; break;
		}
	}

	/* if just one letter was output, follow it by '; Otherwise, put
	 * a " before the last letter */
	if(buf[0]!='\0') {
		if(buf[1]=='\0'){
			buf[1]='\'';
			buf[2]='\0';
		/* NOTE: this test is to make 5001 was ה'א', not ה'א.
		 * I'm not sure this is warranted, but it's what we had in
		 * hspell.pl. Note that b[-2] exists because of the previous
		 * test. */
		} else if(b[-2]=='\'' && b[-1]!='\'') {
			b[0]='\'';
			b[1]='\0';
		} else if(b[-1]!='\'') { /* no " in ה' */
			char save=b[-1];
			b[-1]='"';
			b[0]=save;
			b[1]='\0';
		}
	}
	if(hspell_debug) fprintf(stderr,"returning %s\n",buf);
}
/* TODO: stuff like טו' is now recognized as 15,000. In hspell.pl this
 * wasn't recognized because (I think) a bug in int2gim which generate
 * something like טו"'. Frankly, I doubt we want to recognize this case
 * at all... */
unsigned int
hspell_is_canonic_gimatria(const char *w)
{
	const char *p;
	char buf[50];
	unsigned int val;
	/* make a quick look for quotes (if there are none, this is no
	 * gimatria and we return 0 */
	for(p=w; *p && *p!='"' && *p!='\''; p++)
		;
	if(!*p)
		return 0;
	/* Now make the actual test for canonic gimatria */
	int2gim((val=gim2int(w)), buf);
	if(strcmp(w, buf)) val=0;
	return val;
}
