#!/usr/bin/perl -w
# Copyright (C) 2002-2004 Nadav Har'El and Dan Kenigsberg
#
# converts the textual linguistic information in wolig-d-dictionaries,
# into binary data.
# Usage: cat dict1 dict2 ... | binarize-desc.pl > dict_bin_out
#

use Carp;

require "PrefixBits.pl";

# "perl -w" warns about variables only used once (it assumes they are a
# typo). This ugliness gets rid of this warning. Is there a more sensible way?
($PS_L,$PS_B,$PS_VERB,$PS_NONDEF,$PS_IMPER,$PS_MISC)=
	($PS_L,$PS_B,$PS_VERB,$PS_NONDEF,$PS_IMPER,$PS_MISC);

my $specifier;

############ description handlng

my $D_NOUN=1;
my $D_VERB=2*$D_NOUN;
my $D_ADJ=3*$D_NOUN;
my $D_TYPEMASK=3*$D_NOUN;

my $D_GENDERBASE=4*$D_NOUN;
my $D_MASCULINE=1*$D_GENDERBASE;
my $D_FEMININE=2*$D_MASCULINE;
my $D_GENDERMASK=3*$D_GENDERBASE;

my $D_GUFBASE=4*$D_GENDERBASE;
my $D_FIRST=$D_GUFBASE*1;
my $D_SECOND=$D_GUFBASE*2;
my $D_THIRD=$D_GUFBASE*3;
my $D_GUFMASK=$D_GUFBASE*3;

my $D_NUMBASE=4*$D_GUFBASE;
my $D_SINGULAR=1*$D_NUMBASE;
my $D_DOUBLE=2*$D_NUMBASE;
my $D_PLURAL=3*$D_NUMBASE;
my $D_NUMMASK=3*$D_NUMBASE;

my $D_TENSEBASE=4*$D_NUMBASE;
my $D_INFINITIVE=1*$D_TENSEBASE;
my $D_BINFINITIVE=6*$D_TENSEBASE;
my $D_PAST=2*$D_TENSEBASE;
my $D_PRESENT=3*$D_TENSEBASE;
my $D_FUTURE=4*$D_TENSEBASE;
my $D_IMPERATIVE=5*$D_TENSEBASE;
my $D_TENSEMASK=7*$D_TENSEBASE;

my $D_OGENDERBASE=8*$D_TENSEBASE;
my $D_OMASCULINE=1*$D_OGENDERBASE;
my $D_OFEMININE=2*$D_OMASCULINE;
my $D_OGENDERMASK=3*$D_OGENDERBASE;

my $D_OGUFBASE=4*$D_OGENDERBASE;
my $D_OFIRST=$D_OGUFBASE*1;
my $D_OSECOND=$D_OGUFBASE*2;
my $D_OTHIRD=$D_OGUFBASE*3;
my $D_OGUFMASK=3*$D_OGUFBASE;

my $D_ONUMBASE=4*$D_OGUFBASE;
my $D_OSINGULAR=1*$D_ONUMBASE;
my $D_ODOUBLE=2*$D_ONUMBASE;
my $D_OPLURAL=3*$D_ONUMBASE;
my $D_ONUMMASK=3*$D_ONUMBASE;
my $D_OMASK=3*$D_ONUMBASE+3*$D_OGUFBASE+3*$D_OMASCULINE;

my $D_OSMICHUT=4*$D_ONUMBASE;
my $D_SPECNOUN=2*$D_OSMICHUT;
my $D_STARTBIT=2*$D_SPECNOUN;
#print STDERR "finalbit $D_STARTBIT\n";

sub text2mask {
	my $dmask = 0;
	my $desc = shift;
	return 0 if !$desc;
	if($desc=~m/^([^א-ת]|^)פ([^א-ת]|$)/o) {$dmask |= $D_VERB}
	elsif($desc=~m/([^א-ת]|^)ע([^א-ת]|$)/o) {$dmask |= $D_NOUN}
	elsif($desc=~m/([^א-ת]|^)ת([^א-ת]|$)/o) {$dmask |= $D_ADJ}

	if($desc=~m/,עבר([^א-ת]|$)/o) {$dmask |= $D_PAST}
	elsif($desc=~m/,הווה([^א-ת]|$)/o) {$dmask |= $D_PRESENT}
	elsif($desc=~m/,עתיד([^א-ת]|$)/o) {$dmask |= $D_FUTURE}
	elsif($desc=~m/,ציווי([^א-ת]|$)/o) {$dmask |= $D_IMPERATIVE}
	elsif($desc=~m/,מקור([^א-ת]|$)/o) {$dmask |= $D_INFINITIVE}

	if($desc=~m/,יחיד([^א-ת]|$)/o) {$dmask |= $D_SINGULAR}
	elsif($desc=~m/,רבים([^א-ת]|$)/o) {$dmask |= $D_PLURAL}

	if($desc=~m/([^א-ת]|^)ז([^א-ת]|$)/o) {$dmask |= $D_MASCULINE};
	if($desc=~m/([^א-ת]|^)נ([^א-ת]|$)/o) {$dmask |= $D_FEMININE};

	if($desc=~m/,אני/o) {$dmask |= $D_FIRST | $D_SINGULAR}
	elsif($desc=~m/,אתה/o) {$dmask |= $D_SECOND | $D_SINGULAR | $D_MASCULINE}
	elsif($desc=~m/,את([^א-ת]|$)/o) {$dmask |= $D_SECOND | $D_SINGULAR | $D_FEMININE}
	elsif($desc=~m/,הוא([^א-ת]|$)/o) {$dmask |= $D_THIRD | $D_SINGULAR | $D_MASCULINE}
	elsif($desc=~m/,היא([^א-ת]|$)/o) {$dmask |= $D_THIRD | $D_SINGULAR | $D_FEMININE}
	elsif($desc=~m/,אנו/o) {$dmask |= $D_FIRST | $D_PLURAL}
	elsif($desc=~m/,אתם/o) {$dmask |= $D_SECOND | $D_PLURAL | $D_MASCULINE}
	elsif($desc=~m/,אתן([^א-ת]|$)/o) {$dmask |= $D_SECOND | $D_PLURAL | $D_FEMININE}
	elsif($desc=~m/,הם([^א-ת]|$)/o) {$dmask |= $D_THIRD | $D_PLURAL | $D_MASCULINE}
	elsif($desc=~m/,הן([^א-ת]|$)/o) {$dmask |= $D_THIRD | $D_PLURAL | $D_FEMININE}

	if($desc=~m!/אני!o) {$dmask |= $D_OFIRST | $D_OSINGULAR}
	elsif($desc=~m!/אתה!o) {$dmask |= $D_OSECOND | $D_OSINGULAR | $D_OMASCULINE}
	elsif($desc=~m!/את([^א-ת]|$)!o) {$dmask |= $D_OSECOND | $D_OSINGULAR | $D_OFEMININE}
	elsif($desc=~m!/הוא([^א-ת]|$)!o) {$dmask |= $D_OTHIRD | $D_OSINGULAR | $D_OMASCULINE}
	elsif($desc=~m!/היא([^א-ת]|$)!o) {$dmask |= $D_OTHIRD | $D_OSINGULAR | $D_OFEMININE}
	elsif($desc=~m!/אנו!o) {$dmask |= $D_OFIRST | $D_OPLURAL}
	elsif($desc=~m!/אנחנו!o) {$dmask |= $D_OFIRST | $D_OPLURAL}
	elsif($desc=~m!/אתם!o) {$dmask |= $D_OSECOND | $D_OPLURAL | $D_OMASCULINE}
	elsif($desc=~m!/אתן([^א-ת]|$)!o) {$dmask |= $D_OSECOND | $D_OPLURAL | $D_OFEMININE}
	elsif($desc=~m!/הם([^א-ת]|$)!o) {$dmask |= $D_OTHIRD | $D_OPLURAL | $D_OMASCULINE}
	elsif($desc=~m!/הן([^א-ת]|$)!o) {$dmask |= $D_OTHIRD | $D_OPLURAL | $D_OFEMININE}

	if($desc=~m!סמיכות!o) {$dmask |= $D_OSMICHUT}
	if($desc=~m!פרטי!o) {$dmask |= $D_SPECNOUN}
	return $dmask;
}

sub mask2text {
	my $dmask = shift;
	my $s;
	return "" if !$dmask;
	$s = ${ {$D_NOUN=>'ע', $D_VERB=>'פ', $D_ADJ=>'ת', 0=>'' } }{ ($dmask & $D_TYPEMASK) };
	$s .= ${ {$D_MASCULINE=>',ז', $D_FEMININE=>',נ', 0=>'' } }
		{ ($dmask & $D_GENDERMASK) };
	$s .= ${ {$D_FIRST=>',1', $D_SECOND=>',2', $D_THIRD=>',3', 0=>'' } }{ ($dmask & $D_GUFMASK) };
	$s .= ${ {$D_SINGULAR=>',יחיד', $D_DOUBLE=>',זוגי', $D_PLURAL=>',רבים', 0=>'' } }{ ($dmask & $D_NUMMASK) };
	$s .= ${ {$D_PAST=>',עבר', $D_PRESENT=>',הווה', $D_FUTURE=>',עתיד', $D_IMPERATIVE=>',ציווי', $D_INFINITIVE=>',מקור', $D_BINFINITIVE=>',מקור,ב,', 0=>'' } }{ ($dmask & $D_TENSEMASK) };
	$s .= ",פרטי" if ($dmask & $D_SPECNOUN);
	$s .= ",סמיכות" if ($dmask & $D_OSMICHUT);
	if ($dmask & $D_OMASK) {
		$s .= ",כינוי/".${ {$D_OMASCULINE=>',ז', $D_OFEMININE=>',נ', 0=>'' } }{ ($dmask & $D_OGENDERMASK) };
		$s .= ${ {$D_OFIRST=>',1', $D_OSECOND=>',2', $D_OTHIRD=>',3', 0=>'' } }{ ($dmask & $D_OGUFMASK) };
		$s .= ${ {$D_OSINGULAR=>',יחיד', $D_ODOUBLE=>',זוגי', $D_OPLURAL=>',רבים', 0=>'' } }{ ($dmask & $D_ONUMMASK) };	
	}
	return $s;
}

my (%pack_desc_hash,@dmasks,$stem);
############

print STDERR "reading input dictionaries...\n";
my $c=0;
while(<>){
	chomp;
	#next if /---/;  # TODO: this isn't needed. remove it.
	#s/-$//o; # TODO: dan added this. remove it.
	s/\+ / /o; # The Makefile was supposed to remove those, but still...
	if(/^L/o){
	  $specifier = $PS_L;
	  s/^L//o;
	} elsif(/^B/o){
	  $specifier = $PS_B;
	  s/^B//o;
	} elsif(!/^[א-ת]/o){
	  $stem = undef if m/---/;
	  next; # not a word
	} elsif(/-$/o){
	  # In wolig.pl's simple output (without -d), this specified smichut,
	  # and we shouldn't allow prefixes with he hayedia. This case is
	  # useful for smichut words in extrawords.
	  $specifier = $PS_NONDEF;
	  s/-$//o;
	} elsif(/ פ,/o) {
	  if(/ .*ציווי/o) {
		$specifier = $PS_IMPER;
	  } elsif(!/ .*הווה/o) {
		$specifier = $PS_VERB;
	  } elsif(/ .*סמיכות/o || m:,כינוי/:o) {
		$specifier = $PS_NONDEF;
	  } else {
		$specifier = $PS_ALL;
	  }
	} elsif(/[ ,][עת],/) {
	  if (/ .*סמיכות/o || m:,של/:o || / .*פרטי/o) {
		$specifier = $PS_NONDEF;
	  } else {
		$specifier = $PS_ALL;
	  }
	} else {
	  $specifier = $PS_ALL;
	}
	s/ (.*)$//;	# remove all the "-d" explanations after the word
	$stem = $_ if !defined($stem);
#	$specifiers{$_} |= $specifier;

	my $dmask = defined($1) ? &text2mask($1) : 0;
	$dmask = $dmask & ~$D_INFINITIVE | $D_BINFINITIVE if $specifier==$PS_B;
	my $dcode = $pack_desc_hash{$dmask};
	if (!$dcode) {
		my $i = 0+keys(%pack_desc_hash);
		$dcode = chr(ord('A')+$i%26).chr(ord('A')+($i - $i%26)/26);
		$pack_desc_hash{$dmask} = $dcode;
		push @dmasks, $dmask;
	}
	print "$_\t$specifier\t$dcode\t$stem\n";
	$c++;
	print STDERR "#" if !($c%1000);
}
print STDERR "\ncreate dmask.c...\n";

open(DESC_C,">dmask.c") or die "cannot create dmask.c\n";
print DESC_C "/* This file is automatically generated by binarize-desc.pl.\n".
	     "   DO NOT EDIT THIS FILE DIRECTLY!\n*/\n";
print DESC_C "int dmasks[] = {\n";
print DESC_C join(",\n", @dmasks),"\n};\n";

# the following segment was generate from this very perl code using
# grep '^my $D_' pmerge-bin | perl -pe 's/^my \$(D_.*)=.*$/#define $1 \$${1}/;'

print DESC_C << "EOF"
#define D_NOUN $D_NOUN
#define D_VERB $D_VERB
#define D_ADJ $D_ADJ
#define D_TYPEMASK $D_TYPEMASK
#define D_GENDERBASE $D_GENDERBASE
#define D_MASCULINE $D_MASCULINE
#define D_FEMININE $D_FEMININE
#define D_GENDERMASK $D_GENDERMASK
#define D_GUFBASE $D_GUFBASE
#define D_FIRST $D_FIRST
#define D_SECOND $D_SECOND
#define D_THIRD $D_THIRD
#define D_GUFMASK $D_GUFMASK
#define D_NUMBASE $D_NUMBASE
#define D_SINGULAR $D_SINGULAR
#define D_DOUBLE $D_DOUBLE
#define D_PLURAL $D_PLURAL
#define D_NUMMASK $D_NUMMASK
#define D_TENSEBASE $D_TENSEBASE
#define D_INFINITIVE $D_INFINITIVE
#define D_BINFINITIVE $D_BINFINITIVE
#define D_PAST $D_PAST
#define D_PRESENT $D_PRESENT
#define D_FUTURE $D_FUTURE
#define D_IMPERATIVE $D_IMPERATIVE
#define D_TENSEMASK $D_TENSEMASK
#define D_OGENDERBASE $D_OGENDERBASE
#define D_OMASCULINE $D_OMASCULINE
#define D_OFEMININE $D_OFEMININE
#define D_OGENDERMASK $D_OGENDERMASK
#define D_OGUFBASE $D_OGUFBASE
#define D_OFIRST $D_OFIRST
#define D_OSECOND $D_OSECOND
#define D_OTHIRD $D_OTHIRD
#define D_OGUFMASK $D_OGUFMASK
#define D_ONUMBASE $D_ONUMBASE
#define D_OSINGULAR $D_OSINGULAR
#define D_ODOUBLE $D_ODOUBLE
#define D_OPLURAL $D_OPLURAL
#define D_ONUMMASK $D_ONUMMASK
#define D_OMASK $D_OMASK
#define D_OSMICHUT $D_OSMICHUT
#define D_SPECNOUN $D_SPECNOUN
#define D_STARTBIT $D_STARTBIT

#define PS_ALL $PS_ALL
#define PS_B $PS_B
#define PS_L $PS_L
#define PS_VERB $PS_VERB
#define PS_NONDEF $PS_NONDEF
#define PS_IMPER $PS_IMPER
#define PS_MISC $PS_MISC

EOF
;

close DESC_C;

