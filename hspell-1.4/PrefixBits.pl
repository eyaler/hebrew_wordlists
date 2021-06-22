# Prefix specifiers currently generated:
 $PS_ALL=63;   # All legal prefixes are allowed for this word
 $PS_B=1;      # only certain prefixes ending in bet are allowed.
 $PS_L=2;      # only prefixes ending in bachla"m are allowed (note that if genprefixes.pl gives a certain prefix PS_L, it should also give it PS_B).
 $PS_VERB=4;
 $PS_NONDEF=8;    # accept prefixes w/o ä
 $PS_IMPER=16;    # accept nothing/å
 $PS_MISC=32;
# These have to be bitmasks that can be or'ed easily, so that if one word
# can get prefixes of two types, it will have one combined prefix specifier
# that describes the prefixes.
#
# These prefix specifiers are used by genprefixes.pl to create prefixes.c
# that is used by hspell.c

