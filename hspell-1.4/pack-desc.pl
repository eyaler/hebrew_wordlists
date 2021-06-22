#!/usr/bin/perl -w
# Copyright (C) 2002-2003 Nadav Har'El and Dan Kenigsberg
#
# Packs a sorted, binarized wordlist into, into one dictionary with
# or'ed prefix hints, a description file, and a stem file.
# Usage: cat sorted_bin |
#	pack-desc.pl -p prefixesout -d descout -s stemsout > wordsout
#

use Carp;


use Getopt::Std;
my %opts;
# -p - output prefix file.
# -d - output description file.
# -s - output stems file.
# -l - output dictionary sizes that are relevant to linginfo
if(!getopts('p:d:s:l:', \%opts)){
	exit(1);
}

my $out_prefixes=$opts{p};
my $out_desc=$opts{d};
my $out_stems=$opts{s};
my $out_lingsizes=$opts{l};

my ($lastword,$lastspecifiers, $flatlen);
my ($stems,$dmasks,$specifiers,$c,%pointers);
my @words;
$c=0;
while(<>){
	# I want to print this message only *after* the script started reading
	# its input - but only once.
	print STDERR "reading input sorted flat file...\n"
		 if !defined($lastword);
	chomp;
	next unless m/^(.*)\t(.*)\t(.*)\t(.*)$/;
	my ($word,$specifier,$dmask,$stem) = ($1,$2,$3,$4);
	# found a new word. output the packed one.
	if($lastword && $lastword ne $word) {
		$pointers{$lastword} = $c;
		$stems =~ s/:$//o;
		push @words, "$lastword\t$lastspecifiers\t$dmasks\t$stems";
#		$flatlen += $#lastword+2+($#dmasks+1)/2*5+2;
		$stems="";$dmasks="";$specifiers=0;$c++;
		# it takes ages. let's notify the package builder.
		print STDERR "#" if !($c%1000) ;
	}
	$stems .= "$stem:";
	$dmasks .= $dmask;
	$specifiers |= $specifier;

	$lastword = $word;
	$lastspecifiers = $specifiers;
}
# reached EOF. output the final word. TODO: don't do this ugly copy-paste of
# code. call a proper function!
		$pointers{$lastword} = $c;
		$stems =~ s/:$//o;
		push @words, "$lastword\t$lastspecifiers\t$dmasks\t$stems";
		$stems="";$dmasks="";$specifiers=0;$c++;

print STDERR "\nwriting output files...";
if ($out_prefixes) {
	open(PREFIXES,">$out_prefixes")
		or croak "Couldn't write -p parameter '$out_prefixes'";
}
if ($out_desc) {
	open(DESCS,">$out_desc")
		or croak "Couldn't write -d parameter '$out_desc'";
}
if ($out_stems) {
	open(STEMS,">$out_stems")
		or croak "Couldn't write -s parameter '$out_stems'";
}

foreach (@words) {
	m/^(.*)\t(.*)\t(.*)\t(.*)$/o;
	my ($word,$specifier,$dcodes,$stems) =($1,$2,$3,$4);
	print $word,"\n";
	print PREFIXES chr($specifier) if $out_prefixes;
	print DESCS $dcodes,"\n" if $out_desc;
	$flatlen += length($word)+1;
	if ($out_stems) {
		foreach (split(':',$stems)) {
			my $i = $pointers{$_};
			my $c1 = $i%94;
			my $c2 = ($i-$c1)/94%94;
			my $c3 = ($i-$c1-$c2)/94/94;
			print STEMS chr(33+$c1).chr(33+$c2).chr(33+$c3);
			$flatlen += 5;
		}
		print STEMS "\n";
		$flatlen += 2;
	}
}

close PREFIXES if $out_prefixes;
close DESCS if $out_desc;
close STEMS if $out_stems;

print STDERR "creating $out_lingsizes...\n";

open(DESC_SIZES,">$out_lingsizes") or die "cannot write $out_lingsizes\n";
#print DESC_SIZES "#define FLATSIZE ".$flatlen."\n";
#print DESC_SIZES "#define LOOKUPLEN ".($#words+1)."\n";
print DESC_SIZES $flatlen." ".($#words+1)."\n";
close DESC_SIZES;

