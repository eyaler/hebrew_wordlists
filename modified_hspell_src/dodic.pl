#!/usr/bin/perl
use warnings;
use strict;
use open qw( :encoding(UTF-8) :std );
#use utf8;
use Data::Dumper;

#
# program to unmunch hunspell words using .aff files
# Usage: perl dodic.pl [-s] [-p] [-d=0..10] en_US.aff wordfile
#  -s means: print suffixes only
#  -p means: print prefixes only
#  -d=0..10 means: set debug level
#  where wordfile contains for example: editor/MS
#  all used files must be utf-8 coded
#


my $debug = 0;

my $explanation = "dodic.pl [-s] [-p] [-d=0..10] affixfile.aff wordfile \n-s means: print suffixes only\n-p means:print prefixes only\n-d=0..10 means: set debug level\nwordfile contains the word to be unmunched with the flags you want to use\n for example cica/AUQc\nall files must be UTF-8 coded\n use for example recode l2..u8 for the Hungarian .aff/.dic files \n";

my ($r_suffix, $r_prefix, $affixfile, $is_aff, $wordfile);
$r_suffix = 0;
$r_prefix = 0;
$is_aff = 0;
$affixfile = '';
$wordfile = '';

while(my $arg=shift @ARGV){
       if (substr($arg,0,1) eq '-'){ 
           if(substr($arg, 1,1 ) eq 's'){
             $r_suffix = 1;
           }elsif(substr($arg,1,1) eq 'p'){
             $r_prefix = 1;
           }elsif(substr($arg,1,1) eq 'd'){
             $debug = substr($arg, 3);
             if(!defined($debug)){
               $debug = 0;
             }
          } else{
            die $explanation;
          }
        } elsif($is_aff eq 0){
              $affixfile = $arg;
              $is_aff = 1;
        }else{
              $wordfile = $arg;
        }
        if($r_suffix == 0 and $r_prefix == 0){
          $r_suffix = 1;
          $r_prefix = 1;
        }
        if($debug >= 3){
            printf "%s\n",$arg;
        }
}
if($debug >= 2){
  print "r_s:$r_suffix r_p:$r_prefix deb:$debug af:$affixfile wf:$wordfile\n";
}


my (@sfx_arr, @pfx_arr);

#my($affixfile, $wordfile) =  @ARGV;

if(not defined $affixfile or not defined $wordfile){
   die $explanation;
}

my ($sfxptr, $hashptr);

($sfxptr,$hashptr) = read_in_sfx($affixfile);

open(FH, '<', $wordfile) or die $!;

while(<FH>){
  my @warr = split(/\//, $_);
  my $szo = $warr[0];
  my $flags = $warr[1]; 
  my @flarr = split(//, $flags);
  foreach(@flarr){
    # get sfx index
     my $idx = $hashptr->{$_};
    if(defined($idx)){
      if($debug >= 2){
      print "tag = $_ idx:$idx\n";
      }
      my $count = $sfxptr->[$idx]{'count'};
      my $type  = $sfxptr->[$idx]{'type'};
      my $comb  = $sfxptr->[$idx]{'comb'};
    #   print "idx:$idx cnt=$count\n";
      for (my $i=0; $i < $count; $i++){
        my ($strip, $addtoword, $cond);
        $strip     = $sfxptr->[$idx]{'elements'}->[$i]{'strip'};
        $addtoword = $sfxptr->[$idx]{'elements'}->[$i]{'add_to_word'};
        $cond      = $sfxptr->[$idx]{'elements'}->[$i]{'condition'};
         if($debug >=3){
         print "idx:$idx cnt=$count strip:$strip atw:$addtoword cond:$cond->[0]\n";
         }
         if(met_cond($szo, $cond, $type)){
             my $ujszo;
             if($type eq 's'){
               $ujszo = strip_add_sfx($szo, $strip, $addtoword);
               push(@sfx_arr,$ujszo );
             } elsif($type  eq 'p'){
                if($comb eq 'y' or $comb eq 'Y'){
                  push( @pfx_arr, $addtoword);
                } else{
                  $ujszo = strip_add_pfx($szo, $strip, $addtoword);
                }
             }
             if($r_suffix and defined($ujszo)){print "$ujszo\n";}
         } 
      }
    }
  }
  if($r_prefix){
    foreach(@pfx_arr){
      my $pfx = $_;
      foreach(@sfx_arr){
        my $ujszo = $pfx.$_;
        if(defined($ujszo)){print "$ujszo\n";}
      }
    }
  }
    
}

close(FH);



sub read_in_sfx{
   my($affixfile) = @_; 
   
 my $new = 1;
 my (@sfx);
 my ($idx);
 $idx = 0;
 my $counter = 0;
 #my $debug = 2;
 my %shash;
 
open(FH, '<', $affixfile) or die $!;

 while(<FH>){
   if(index($_, "SFX ") == 0 or index($_, "PFX ") == 0){
     if($debug >=3){
       print $_;
     }
     if($new){
         my @fields = split( /\s{1,}/, $_);
         my @newarr;
        # print Dumper (\@fields);
         $sfx[$idx]{'count'}    = $fields[3];
         $sfx[$idx]{'id'}       = $fields[1];
         $sfx[$idx]{'comb'}     = $fields[2];
         $shash{$fields[1]} = $idx;
         if($fields[0] eq 'SFX'){
           $sfx[$idx]{'type'}     = 's';
         } else{
           $sfx[$idx]{'type'}     = 'p';
          }
         $sfx[$idx]{'elements'} = \@newarr;
         $new = 0;
      } else{
        my @fields = split( /\s{1,}/, $_);
        my $r = $sfx[$idx]{'elements'};
        my @newarr = @$r;
        $newarr[$counter]{'strip'}         = $fields[2];
        #
        # strip /.. from prefix
        #
        my @tmparr = split(/\//, $fields[3]);
        $newarr[$counter]{'add_to_word'}   = $tmparr[0];
        $newarr[$counter]{'condition'}     = read_cond($fields[4]);
        $sfx[$idx]{'elements'} = \@newarr;
         ++ $counter;
        if($counter eq $sfx[$idx]{'count'}){
            $new = 1;
            $counter = 0;
            ++$idx;
         }
         
     }
   }
 }

 close(FH);

 return (\@sfx, \%shash);
}


sub read_cond{
   my($condition) = @_; 
   
   my @carr;
   
   my $in_loop = 0;
   my @condarr = split(//, $condition);
   my ($tcarr);
   foreach (@condarr){
      if ($_  eq '['){
         if(!$in_loop){
           $in_loop = 1;
          } else {
            print "error1 in condition $condition\n";
          }
      } 
      elsif($_ eq ']'){
        if($in_loop) {
          push(@carr, $tcarr);
          $in_loop = 0;
          $tcarr = '';
        }else {
            print "error2 in condition $condition\n";
        }
      }else {
        if($in_loop){
          $tcarr .= $_;
        }else{
           push(@carr, $_);
        }
      }
         
   }
   if($debug >=3){
      my $condarrsize =  @carr;
      my  $i;
      print "carr: $condarrsize\n";
      for ($i = 0; $i < $condarrsize; $i++){
        print "$i $carr[$i]\n";
      }
   }
   return \@carr;
   
}

sub met_cond{
   my($szo, $condref, $type) = @_; 
   
   my @carr = @$condref;
   my $condarrsize =  @carr;
   
   if($carr[0] eq '.'){
     return 1;
   }elsif ($type eq 's'){
   my $lszo = length($szo);
   my $szoidx = $lszo - 1;
   my $i;
   for($i = $condarrsize -1; $i >=0; $i--){
     my $tobechecked = substr($szo, $szoidx, 1);
     if($debug >= 4){
        print "tbc:$tobechecked szdx:$szoidx ci:$carr[$i]\n";
     }
     if(length($carr[$i]) == 1){
        if ( $carr[$i] ne $tobechecked ){
           if($debug >= 3){
             print "no match1\n";
           }
           return 0;
        }
    } else{
      my $j ;
      my $matched = 0;
      my $clen = length($carr[$i]);
      if(substr($carr[$i],0,1) eq '^'){ # inverted check
         for($j = 1; $j < $clen; $j++){
           if(substr($carr[$i],$j,1) eq $tobechecked){
             if($debug >= 3){
               print "no match2\n";
             }
             return 0;
           }
         }
         $matched = 1;
        } else{ # at least one matches
         for($j = 1; $j < $clen; $j++){
           if(substr($carr[$i],$j,1) eq  $tobechecked){
             $matched = 1;
             last;
           }
         }
        }
        if($matched eq 0){
           if($debug >= 3){
            print "no match3 i= $i szi: $szoidx tbc:$tobechecked\n";
           }
            return 0;
        }
     }
     --$szoidx;
    }          
      
   } elsif($type eq 'p'){
   my $szoidx = 0;
   my $i;
   for($i = 0; $i <= $condarrsize -1; $i++){
     my $tobechecked = substr($szo, $szoidx, 1);
     if($debug >= 4){
        print "tbc:$tobechecked szdx:$szoidx ci:$carr[$i]\n";
     }
     if(length($carr[$i]) == 1){
        if ( $carr[$i] ne $tobechecked ){
           if($debug >= 3){
             print "no match1\n";
           }
           return 0;
        }
    } else{
      my $j ;
      my $matched = 0;
      my $clen = length($carr[$i]);
      if(substr($carr[$i],0,1) eq '^'){ # inverted check
         for($j = 1; $j < $clen; $j++){
           if(substr($carr[$i],$j,1) eq $tobechecked){
             if($debug >= 3){
               print "no match2\n";
             }
             return 0;
           }
         }
         $matched = 1;
        } else{ # at least one matches
         for($j = 1; $j < $clen; $j++){
           if(substr($carr[$i],$j,1) eq  $tobechecked){
             $matched = 1;
             last;
           }
         }
        }
        if($matched eq 0){
           if($debug >= 3){
            print "no match3 i= $i szi: $szoidx tbc:$tobechecked\n";
           }
            return 0;
        }
     }
     ++$szoidx;
    }          
      

   
   
   
   
   
   
   }
  return 1;  
   
}
sub strip_add_sfx{
  my($szo, $strip, $atw) = @_;
  if($strip ne '0'){ 
    $szo =  substr($szo, 0, (length($szo)-length($strip)));
  }
  return $szo.$atw;

}
sub strip_add_pfx{
  my($szo, $strip, $atw) = @_;
  if($strip ne '0'){ 
    $szo =  substr($szo, 0, (length($szo)-length($strip)));
  }
  return $atw.$szo;

}

