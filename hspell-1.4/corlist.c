/* Copyright (C) 2003 Nadav Har'El and Dan Kenigsberg */

/* a silly implementation of a list of correction words */

#include <string.h>

#include "hspell.h"

int
corlist_init(struct corlist *cl)
{
	cl->n=0;
	return 1;
}

int
corlist_free(struct corlist *cl)
{
	/* no need to do anything in this implementation */
	cl->n=0; /* not necessary */
	return 1;
}

int
corlist_add(struct corlist *cl, const char *s)
{
	int i;
	for(i=0; i<cl->n; i++){
		if(!strcmp(cl->correction[i],s))
			return 1; /* already in list! */
	}
	if(cl->n==(sizeof(cl->correction)/sizeof(cl->correction[0])))
		return 0; /* no room */
	strncpy(cl->correction[cl->n++], s, sizeof(cl->correction[0]));
	return 1;
}
