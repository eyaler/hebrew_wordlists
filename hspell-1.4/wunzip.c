#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	char sbuf[256];
	int slen=0;
	int c,n;
	while((c=getchar())!=EOF){
		if(c>='0' && c<='9'){
			/* new word - output old word first */
			sbuf[slen]='\0';
			puts(sbuf);
			/* and read how much to go back */
			n=0;
			do {
				/* base 10... */
				n*=10;
				n+=(c-'0');
			} while ((c=getchar())!=EOF && c>='0' && c<='9');
			slen-=n;
			if(slen<0 || slen >= sizeof(sbuf)-1){
				fprintf(stderr,"bad backlength %d... exiting.\n", slen);
				exit(1);
			}
			/* we got a new letter c - continue the loop */
		}
		/* word letter - add it */
		if(slen>=sizeof(sbuf)-1){
			fprintf(stderr,"word too long... exiting.\n");
			exit(1);
		}
		sbuf[slen++]=c;
	}
	/* output last word */
	sbuf[slen]='\0';
	puts(sbuf);
	return 0;
}
