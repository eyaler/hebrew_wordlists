/* Copyright (C) 2003 Nadav Har'El and Dan Kenigsberg */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>

#include "dict_radix.h"

#include <time.h>

int
main(int argc, char *argv[])
{
	struct dict_radix *dict = new_dict_radix();

	allocate_nodes(dict, 200000, 100000, 10000);

	read_dict(dict, NULL);
	print_sizes(dict);
	print_stats(dict);
	return 0;
}
