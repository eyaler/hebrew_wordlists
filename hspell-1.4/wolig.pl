#!/usr/bin/perl -w
#
# Copyright (C) 2000-2015 Nadav Har'El, Dan Kenigsberg
#
use Carp;
use FileHandle;

my $detailed_output=0;
my $detail_prefix;

# This arrays will be useful later to convert ordinary letters into final,
# and vice-versa.
my %fin = ('כ'=>'ך', 'מ'=>'ם', 'נ'=>'ן', 'פ'=>'ף', 'צ'=>'ץ');
my %nif = ('ך'=>'כ', 'ם'=>'מ', 'ן'=>'נ', 'ף'=>'פ', 'ץ'=>'צ');

sub outword {
  my $word = shift;
  my $details = shift;

  # "*" sign used to signify non-existant word that should not be output.
  # It will allow us to more-easily drop words without huge if()s.
  return if $word =~ m/^\*/;

  # change otiot-sofiot in the middle of the word
  # (the silly a-z was added for our special "y" and "w" marks).
  # (the ('?) and $2 are for סנדוויץ', סנדוויצ'ים)
  $word =~ s/([ךםןףץ])('?)(?=[א-תa-z])/$nif{$1}$2/go;

  # change special consonant marks into the proper Hebrew letters, using
  # proper ktiv male rules.

  # Note that the order of these conversion is important. Since they have
  # the potential of changing so many words, it is highly recommended to
  # diff the output files before and after the change, to see that no
  # unexpected words got changed.

  # The vowel markers 'a' and 'e' do nothing except to a yud (chirik male) -
  # which turns it into a consonant yud; For example your(feminine) צי is
  # צייך (tsere in the yud, so it's a consonant and doubled) and
  # your(masculine) צי is ציך (yud is chirik male, and not doubled)
  $word =~ s/י[ea]/y/go;
  $word =~ s/[ea]//go;

  # The vowel 'i' is a chirik chaser - it should be followed by a yud if
  # necessary. We do nothing with it currently - it's only useful for words
  # like סנאiי where we want to make sure that wolig.pl does not think this
  # is the normal patach-aleph-yud (with no niqqud under the aleph) case as
  # in תנאי.
  # The first rule here is useful for transformation from שני to שנייה, via
  # שני adj-inword> שנiי feminine> שנiיaה outword> שנiyה outword> שנייה
  $word =~ s/iy/יי/go;  # useful in stuff like שנiי - שנייה
  $word =~ s/i//go;

  # Y is the same as y, except it is not translated to a double-yud (but rather
  # to a single yud) when it is the last letter of the word. It's used in words
  # like חולי in which the original form of the word has a chirik male, but in
  # all the inflections the yud from the chirik becomes a fully-fleged
  # consonant. We do not need a similar trick for vav (w), because the
  # Academia's rules do not do anything to a vav at the end of the word,
  # contrary to what happens to a yud.
  # I'm not sure this trick is "kosher" (based on the language), but it does
  # work...
  $word =~ s/Y($|(?=-))/י/go;  # Y's at the end of the word
  $word =~ s/Y/y/go;       # the rest of the Y's are converted to y's

  # The first conversion below implements the akademia's rule that a chirik
  # before a yו should not be written with a י. So we convert יyו into יו.
  # IDEA: to be more certain that the first י functions as a chirik, it would
  # have been better to use the i character: in addition to the יה -> yה rule
  # we have in the beginning of processing a word, we should do ייה -> iyה.
  # Then here the rule would convert iyו, not יyו. [but everything is working
  # well even without this idea]
  $word =~ s/יyו/יו/go;
  $word =~ s/(?<=[^ויy])y(?=[^ויyה]|$)/יי/go;
  $word =~ s/y/י/go;                      # otherwise, just one yud.

  # The first conversion below of וw to ו has an interesting story. In the
  # original Hebrew, the consonant ו sounded like the English w or Arabic
  # waw. An "u" sound (a kubuts, which we mark by ו) followed by this w
  # sound sounded like a long "u", which was later written with a shuruk,
  # i.e., one vav. This conversion is very useful for understanding how the
  # word שוק is inflected (see explanation in wolig.dat).
  $word =~ s/וw/ו/go;
  $word =~ s/(?<=[^וw])w(?=[^וw-])/וו/go;  # if vav needs to be doubled, do it
  $word =~ s/w/ו/go;                       # otherwise, just one vav.

  # A consonant ה (h) is always output as a ה. The only reason we are
  # interested in which ה is consonant is to allow the rules earlier to double
  # yud next to a consonant ה (i.e.. h), but not next to a em-kria ה.
  # For example, compare אריה (lion) and ארייה (her lion).
  $word =~ s/h/ה/go;

  if($detailed_output && defined($details)){
    $word =~ s/-$//;  # smichut is already known by the details...
    $word .= " ".$detail_prefix.$details;
  }
  print $word."\n";
}

sub inword {
  # For some combinations of אהוי at the end or beginning of a word, we can
  # immediately guess that these must be consonants (and not vowels) and make
  # use of that knowledge by changing the Hebrew letters into the markers
  # "w", "y" we use for consonants ו and י respectively.
  #
  # This function takes a word as inputted from wolig.dat, presumably written
  # in ktiv male, and makes a few predictions, such as that a vav in the
  # beginning of the word must be a consonant. Predictions that appear here
  # must have two traits:
  # 1. They must be useful for the correct inflection of some word.
  #    For example, realising that the וו at the end of מזווה is a consonant
  #    help us later avoid the false inflection מזווו and instead generate
  #    the correct מזוו.
  # 2. They must be correct in 100% of the cases. For example, a rule saying
  #    that every appearance of וו in the input is a consonant (w) is wrong,
  #    because of words like ציווי.
  #    However, the rules only have to "appear" correct (for all the actual
  #    words in wolig.dat), not necessarily be linguisticly correct. For
  #    example, we'll see below a rule that a ו at the end of a word is a
  #    consonant (w). This is indeed true for most nouns (צו, מקווקו), but not
  #    for אחו. However, all of אחו's inflections have a consonant vav, and in
  #    the word itself we don't really care about mislabeling it "consonant"
  #    because a vav at the end of the word isn't doubled anyway under the
  #    Academia's rules.
  #
  # Actually the second rule can be relaxed a bit if we provide alternative
  # ways to input a certain construct. For example, if "u" could signify a
  # vowel vav in the input, then we wouldn't really care if in a few rare cases
  # we wrongly decide a certain vav to be consonant: the user could override
  # this decision by putting a "u" explicitly, instead of the vav, in the
  # input file.

  my $word = shift;
  if(substr($word,0,1) eq "ו"){
    # A word cannot start with a shuruk or kubuts!
    substr($word,0,1)="w";
  }
  if(substr($word,-4,4) eq "וויה"){
    # A word like חוויה, הלוויה, טריוויה. I can't imagine any base noun (or
    # adjective) for which such a double-vav isn't a consonant but rather
    # a vav and shuruk.
    substr($word,-4,2)="w";
  }
  if(substr($word,-1,1) eq "ו"){
    # This vav is a consonant (see comment above about why the few exceptions
    # that do exist don't bother us).
    substr($word,-1,1)="w";
  } elsif(substr($word,-3,3) eq "ווה"){
    # If the word ends with ווה, the user wrote in ktiv male and intended
    # a consonant vav. Replace the וו by the character "w", which will be
    # doubled if necessary (for ktiv male) by outword. This change actually
    # makes a difference for the סגול_ה with ות cases: for example, the
    # word מקווה has a plural מקוות and his-possesive מקוו. Without this
    # change, we get the incorrect possesive מקווו and plural מקווות.
    # Similarly it is needed for the adjective נאווה's correct feminine plural.
    substr($word,-3,2)="w";
  } elsif(substr($word,-2,2) eq "יה"){
    substr($word,-2,1)="y";
    # TODO: maybe convert ייה (in ktiv male, e.g., סופגנייה) into iyה.
    # see outword above on a discussion about that. But everything also
    # works without this change.
  }
  return $word;
}

#############################################################################

my ($fh,$word,$optstring,%opts);

my $infile;
if($#ARGV < 0){
  $infile="wolig.dat";
} else {
  if($ARGV[0] eq "-d"){
    $detailed_output=!$detailed_output;
    shift @ARGV;
  }
  $infile=$ARGV[0];
}

$fh = new FileHandle $infile, "r"
  or croak "Couldn't open data file $infile for reading";
while(<$fh>){
  print if /^#\*/;        # print these comments.
  chomp;
  s/#.*$//o;              # comments start with '#'.
  next if /^[ 	]*$/o;    # ignore blank lines.
  ($word,$optstring)=split;
  die "Type of word '".$word."' was not specified." if !defined($optstring);
  undef %opts;
  my $val;
  foreach $opt (split /,/o, $optstring){
    ($opt, $val) = (split /=/o, $opt);
    $val = 1 unless defined $val;
    $opts{$opt}=$val;
  }
  if($opts{"ע"}){
    ############################# noun ######################################
    # Shortcuts
    if($opts{"אין_נטיות"}){
      $opts{"יחיד"}=1; $opts{"אין_נטיות_יחיד"}=1;
    }
    if($opts{"אין_כינויים"}){
      $opts{"אין_כינויי_יחיד"}=1; $opts{"אין_כינויי_רבים"}=1;
    }
    # note that the noun may have several plural forms (see, for example,
    # אות). When one of the plural forms isn't explicitly specified, wolig
    # tries to guess, based on simplistic heuristics that work for the majority
    # of the nouns (84% of them, at one time I counted).
    my $plural_none = $opts{"יחיד"} || substr($word,-3,3) eq "יות";
    my $plural_bizarre = exists($opts{"רבים"});
    my $plural_implicit = !($opts{"ות"} || $opts{"ים"} || $opts{"יות"}
                           || $opts{"אות"} || $opts{"יים"} || $plural_none
                           || $plural_bizarre);
    my $plural_iot = $opts{"יות"} ||
      ($plural_implicit && (substr($word,-2,2) eq "ות"));
    my $plural_xot = $opts{"אות"};
    my $plural_ot = $opts{"ות"} ||
      ($plural_implicit && !$plural_iot && (substr($word,-1,1) eq "ה" || substr($word,-1,1) eq "ת" ));
    my $plural_im = $opts{"ים"} || ($plural_implicit && !$plural_ot && !$plural_iot);
    my $plural_iim = $opts{"יים"};

    # Find gender for detailed output. This has nothing to do with word
    # inflection, it's just an added value of wolig.pl...
    if($detailed_output){
      my $gender;
      if($opts{"זכר"}){
        if($opts{"נקבה"}){
          $gender="ז,נ";
        } else {
          $gender="ז";
        }
      } elsif($opts{"נקבה"}){
        $gender="נ"
      } elsif($opts{"סגול_ה"}){
        $gender="ז";
      } elsif((substr($word,-1,1) eq "ה") && !$opts{"אבד_ו"}){
        $gender="נ";
      } elsif(substr($word,-1,1) eq "ת" && !$opts{"ים"}){
        $gender="נ";
      } else {
        $gender="ז";
      }
      $detail_prefix="$gender,";
    }

    # preprocess the word the user has given, converting certain ktiv male
    # constructs into markers (w, y) that we can better work with later (see
    # comments in inword() about what it does).
    $word=inword($word);

    # related singular noun forms
    if(exists $opts{"נפרד"}){
      outword $opts{"נפרד"}, "ע,יחיד";  # explicit override of the nifrad
    } elsif(!$opts{"אין_יחיד"}){
      outword $word, "ע,יחיד"; # the singular noun itself
    }
    if($opts{"אבד_י"}){
      # in words like עיפרון and היריון the first yud (coming from chirik
      # or tsere in ktiv male) is lost in all but the base word
      $word =~ s/י//o;
    }
    my $smichut=$word;
    if($opts{"אין_יחיד"} || $opts{"אין_נטיות_יחיד"}){
      # We mark the singular words with "*", telling outword to drop them.
      # This makes the code look cleaner than a huge if statement around all
      # the singular code. Maybe in the future we should move the singular
      # inflection code to a seperate function, if() only around that, and
      # stop all that "*" nonsense.
      $smichut="*".$smichut;
    }
    #my $smichut_orig=$smichut;
    if($opts{"מיוחד_אח"}){
      # special case:
      # אח, אב, חם, פה include an extra yod in the smichut. Note that in the
      # first person singular possessive, we should drop that extra yod.
      # For a "im" plural, it turns out to be the same inflections as the
      # plural - but this is not the case with a "ot" plural.
      # Interestingly, the yud in these inflections is always a chirik
      # male - it is never consonantal (never has a vowel on it).
      if(substr($smichut,-1,1) eq "ה"){
        # Remove the ה. Basically, only one word fits this case: פה
        $smichut=substr($smichut,0,-1);
        # And add the extra third-person masuline possesive (just like the
        # סגול_ה case, but we don't bother to check for the סגול_ה flag here).
        outword $smichut."יהו", "ע,יחיד,של/הוא";
      }
      outword $smichut."י-",  "ע,יחיד,סמיכות"; # smichut
      outword $smichut."י",   "ע,יחיד,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו", "ע,יחיד,של/אנחנו";
      outword $smichut."יך",  "ע,יחיד,של/אתה";
      outword $smichut."יך",  "ע,יחיד,של/את";
      outword $smichut."יכם", "ע,יחיד,של/אתם";
      outword $smichut."יכן", "ע,יחיד,של/אתן";
      outword $smichut."יו",  "ע,יחיד,של/הוא";
      outword $smichut."יה",  "ע,יחיד,של/היא";
      outword $smichut."יהן", "ע,יחיד,של/הן";
      outword $smichut."יהם", "ע,יחיד,של/הם";
    } else {
      if(!$opts{"סגול_ה"}){ # replace final ה by ת, unless סגול_ה option
        if(substr($smichut,-1,1) eq "ה" && !$opts{"סגול_ה"}){
          substr($smichut,-1,1)="ת";
        }
      }
      if(exists($opts{"נסמך"})){
        outword $opts{"נסמך"}."-", "ע,יחיד,סמיכות";
      } else {
        outword $smichut."-", "ע,יחיד,סמיכות"; # smichut
      }
      if($opts{"מיוחד_שן"}){
        # academia's ktiv male rules indicate that the inflections of שן
        # (at least the plural is explicitly mentioned...) should get an
        # extra yud - to make it easy to distinguish from the number שניים.
        substr($smichut,0,-1)=substr($smichut,0,-1).'י';
        substr($word,0,-1)=substr($word,0,-1).'י';
      }
      if(substr($word,-2,2) eq "אי" && length($word)>2){
        # in words ending with patach and then the imot kria aleph yud,
        # such as תנאי and גבאי, all the inflections (beside the base word
        # and the smichut) are as if the yud wasn't there.
        # Note that words ending with אי but not patach, like אי and סנאי,
        # should not get this treatment, so there should be an option to turn
        # it off.
        substr($word,-1,1)="";
        substr($smichut,-1,1)="";
      }
      # Note that the extra vowel markers, 'a' and 'e' are added for mele'im
      # ending with yud (e.g., אי) - this vowel attaches to the yud and makes
      # the yud a consonant. This phenomenon is handled in outword.
      my $no_ah=0;
      if($opts{"סגול_ה"}){
        # the ה is dropped from the singular inflections, except one alternate
        # inflection like מורהו (the long form of מורו):
        # (there's another femenine inflection, מורה with kamats on the he,
        # but this is spelled the same (as מורה with mapik) without niqqud so
        # we don't need to print it again).
        if(substr($smichut,-1,1) eq "ה"){
          $smichut=substr($smichut,0,-1);
        }
	unless ($opts{"אין_כינויי_יחיד"}){
        	outword $smichut."ehו", "ע,יחיד,של/הוא";
	}
        # TODO: maybe add the "eha" inflection? But it won't generate anything
        # different from the ah below...
        #outword $smichut."eha" unless $no_ah;
      }
      unless ($opts{"אין_כינויי_יחיד"}){
      outword $smichut."י",   "ע,יחיד,של/אני"; # possessives (kinu'im)
      outword $smichut."eנו", "ע,יחיד,של/אנחנו";
      outword $smichut."ך",   "ע,יחיד,של/אתה";
      outword $smichut."eך",  "ע,יחיד,של/את";
      outword $smichut."כם",  "ע,יחיד,של/אתם";
      outword $smichut."כן",  "ע,יחיד,של/אתן";
      outword $smichut."ו",   "ע,יחיד,של/הוא";
      outword $smichut."ah",  "ע,יחיד,של/היא";
      outword $smichut."aן",  "ע,יחיד,של/הן";
      outword $smichut."aם",  "ע,יחיד,של/הם";
      }
    }
    # related plural noun forms
    # note: don't combine the $plural_.. ifs, nor use elsif, because some
    # nouns have more than one plural forms.
    if($plural_im){
      my $xword=$word;
      if(substr($xword,-1,1) eq "ה" && !$opts{"שמור_ת"}){
        # remove final "he" (not "tav", unlike the "ot" pluralization below)
        # before adding the "im" pluralization, unless the שמור_ת option was
        # given.
        $xword=substr($xword,0,-1);
      }
      my $xword_orig=$xword;
      if($opts{"אבד_ו"}){
        # when the אבד_ו flag was given,we remove the first "em kri'a" from
        # the word in most of the inflections. (see a discussion of this
        # option in wolig.dat).
        $xword =~ s/ו//o;
      }
      outword $xword."ים", "ע,רבים";
      $smichut=$xword;
      my $smichut_orig=$xword_orig;
      unless ($opts{"אין_נטיות_רבים"}){
      outword $smichut_orig."י-", "ע,רבים,סמיכות"; # smichut
      }
      # (We write patach followed by a consonant yud as "y", and later this will
      # give us the chance to automatically double it as necessary by the
      # Academia's ktiv male rules)
      unless ($opts{"אין_כינויי_רבים"}||$opts{"אין_נטיות_רבים"}){
      outword $smichut."y",        "ע,רבים,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו",      "ע,רבים,של/אנחנו";
      outword $smichut."יך",       "ע,רבים,של/אתה";
      outword $smichut."yך",       "ע,רבים,של/את";
      outword $smichut_orig."יכם", "ע,רבים,של/אתם";
      outword $smichut_orig."יכן", "ע,רבים,של/אתן";
      outword $smichut."יו",       "ע,רבים,של/הוא";
      outword $smichut."יה",       "ע,רבים,של/היא";
      outword $smichut_orig."יהן", "ע,רבים,של/הן";
      outword $smichut_orig."יהם", "ע,רבים,של/הם";
      }
    }
    if($plural_iim || $opts{"זוגי"}){
      # The difference between זוגי and יים is that זוגי adds only the "יים"
      # plural, while יים adds the plural and its inflections. For example,
      # for שנתיים, יומיים, שעתיים, שבועיים, נקודתיים, one would never say
      # שנתיי (my two years); On the other hand for other words יים and all
      # the inflections it implies makes sense, e.g., consider ציפורניים,
      # שפתיים, קרניים.
      my $xword=$word;
      if(substr($xword,-1,1) eq "ה"){
        # Change final he into tav before adding the "iim" pluralization.
        $xword=substr($xword,0,-1)."ת";
      }
      my $xword_orig=$xword;
      outword $xword."yם", "ע,רבים";
      $smichut=$xword;
      my $smichut_orig=$xword_orig;
      unless ($opts{"אין_נטיות_רבים"} || !$plural_iim){
      outword $smichut_orig."י-", "ע,רבים,סמיכות"; # smichut
      }
      unless ($opts{"אין_כינויי_רבים"}||$opts{"אין_נטיות_רבים"} || !$plural_iim){
      outword $smichut."y",        "ע,רבים,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו",      "ע,רבים,של/אנחנו";
      outword $smichut."יך",       "ע,רבים,של/אתה";
      outword $smichut."yך",       "ע,רבים,של/את";
      outword $smichut_orig."יכם", "ע,רבים,של/אתם";
      outword $smichut_orig."יכן", "ע,רבים,של/אתן";
      outword $smichut."יו",       "ע,רבים,של/הוא";
      outword $smichut."יה",       "ע,רבים,של/היא";
      outword $smichut_orig."יהן", "ע,רבים,של/הן";
      outword $smichut_orig."יהם", "ע,רבים,של/הם";
      }
    }
    if($plural_ot){
      my $xword=$word;
      if(substr($xword,-1,1) eq "ה" || substr($xword,-1,1) eq "ת"){
        # remove final "he" or "tav" before adding the "ot" pluralization,
        # unless the שמור_ת option was given.
        if(!$opts{"שמור_ת"}){
          $xword=substr($xword,0,-1);
        }
      }
      if($opts{"אבד_ו"}){
        # In segoliim with cholam chaser chat that inflect like feminines
        # (i.e., the plural_ot case), the cholam is lost *only* in the base
        # plural, not in other plural inflection. This is comparable to the
        # inflections of the word מלכה, where the patach is lost only in the
        # base plural.
        # See for example גורן, דופן.
        my $tmp = $xword;
        $tmp =~ s/ו//o;
        outword $tmp."ות",    "ע,רבים";
      } else {
        outword $xword."ות",  "ע,רבים";
      }

      $smichut=$xword."ות";
      unless ($opts{"אין_נטיות_רבים"}){
      outword $smichut."-",   "ע,רבים,סמיכות"; # smichut
      }
      unless ($opts{"אין_כינויי_רבים"}||$opts{"אין_נטיות_רבים"}){
      outword $smichut."y",   "ע,רבים,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו", "ע,רבים,של/אנחנו";
      outword $smichut."יך",  "ע,רבים,של/אתה";
      outword $smichut."yך",  "ע,רבים,של/את";
      outword $smichut."יכם", "ע,רבים,של/אתם";
      outword $smichut."יכן", "ע,רבים,של/אתן";
      outword $smichut."יו",  "ע,רבים,של/הוא";
      outword $smichut."יה",  "ע,רבים,של/היא";
      outword $smichut."יהן", "ע,רבים,של/הן";
      outword $smichut."יהם", "ע,רבים,של/הם";
      }
    }
    if($plural_iot){
      my $xword=$word;
      if(substr($xword,-1,1) eq "ה" || substr($xword,-1,1) eq "ת"){
        # remove final "he" or "tav" before adding the "iot" pluralization,
        # unless the שמור_ת option was given.
        if(!$opts{"שמור_ת"}){
          $xword=substr($xword,0,-1);
        }
      }
      outword $xword."יות",   "ע,רבים";
      $smichut=$xword."יות";
      unless ($opts{"אין_נטיות_רבים"}){
      outword $smichut."-",   "ע,רבים,סמיכות"; # smichut
      }
      unless ($opts{"אין_כינויי_רבים"}||$opts{"אין_נטיות_רבים"}){
      outword $smichut."y",   "ע,רבים,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו", "ע,רבים,של/אנחנו";
      outword $smichut."יך",  "ע,רבים,של/אתה";
      outword $smichut."yך",  "ע,רבים,של/את";
      outword $smichut."יכם", "ע,רבים,של/אתם";
      outword $smichut."יכן", "ע,רבים,של/אתן";
      outword $smichut."יו",  "ע,רבים,של/הוא";
      outword $smichut."יה",  "ע,רבים,של/היא";
      outword $smichut."יהן", "ע,רבים,של/הן";
      outword $smichut."יהם", "ע,רבים,של/הם";
      }
    }
    if($plural_xot){
      my $xword=$word;
      if(substr($xword,-1,1) eq "ה" || substr($xword,-1,1) eq "ת"){
        # remove final "he" or "tav" before adding the "xot" pluralization,
        # unless the שמור_ת option was given.
        if(!$opts{"שמור_ת"}){
          $xword=substr($xword,0,-1);
        }
      }
      outword $xword."אות",   "ע,רבים";
      $smichut=$xword."אות";
      unless ($opts{"אין_נטיות_רבים"}){
      outword $smichut."-",   "ע,רבים,סמיכות"; # smichut
      }
      unless ($opts{"אין_כינויי_רבים"}||$opts{"אין_נטיות_רבים"}){
      outword $smichut."y",   "ע,רבים,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו", "ע,רבים,של/אנחנו";
      outword $smichut."יך",  "ע,רבים,של/אתה";
      outword $smichut."yך",  "ע,רבים,של/את";
      outword $smichut."יכם", "ע,רבים,של/אתם";
      outword $smichut."יכן", "ע,רבים,של/אתן";
      outword $smichut."יו",  "ע,רבים,של/הוא";
      outword $smichut."יה",  "ע,רבים,של/היא";
      outword $smichut."יהן", "ע,רבים,של/הן";
      outword $smichut."יהם", "ע,רבים,של/הם";
      }
    }
    if($plural_bizarre){
      # User specified plural for bizarre cases; For example, the plural of
      # צל is צללים, the plural of בת is בנות.
      # We take the fully formed plural from the user, and may need to take
      # of the ending to guess the smichut and possesives (letting the user
      # override the smichut forms too).
      my $plural=$opts{"רבים"};
      #outword $plural, "ע,רבים";
      outword((exists($opts{"נפרדים"}) ? $opts{"נפרדים"} : $plural), "ע,רבים");
      # Overriding the plural nishmach with the נסמכים option: David Yalin,
      # In his book דקדוק הלשון העברית (1942) explains in page 207 how some
      # of the kinuyim are known as "kinuyey hanifrad" and some "kinuyey
      # hanishmach" because when the nismach and nifrad differ, they follow
      # different ones. This is important for words like תיש, and in fact
      # the אבד_ו option does basically the same thing.
      my $smichut_orig;
      unless ($opts{"אין_נטיות_רבים"}){
      if(substr($plural,-2,2) eq "ות"){
        $smichut_orig= exists($opts{"נסמכים"}) ? $opts{"נסמכים"} : $plural;
        # as David Yalin explains (ibid.): "צריך להעיר כי בשמות שסימן הריבוי
        # שלהם הוא -ות נוטים כל כינויי הרבים אחרי צורת הסמיכות".
        $smichut=$smichut_orig;
        outword $smichut_orig."-", "ע,רבים,סמיכות"; # smichut
      } elsif(substr($plural,-2,2) eq "ים" || substr($plural,-2,2) eq "ין"){
        $smichut=substr($plural,0,-2);
        # the removal of the final yod from נסמכים is a bit silly... maybe
        # we should have had a מקור_נסמכים option and ask it without yod.
        $smichut_orig= exists($opts{"נסמכים"}) ?
          substr($opts{"נסמכים"},0,-1) : $smichut;
        outword $smichut_orig."י-", "ע,רבים,סמיכות"; # smichut
      } else {
        #die "Plural given for $word is of unrecognized form: $plural.";
        # An unrecognized plural form, so we don't know how to construct the
        # construct forms from it. Just ignore them.
        $opts{"אין_כינויי_רבים"}=1;
      }
      }
      unless ($opts{"אין_כינויי_רבים"}||$opts{"אין_נטיות_רבים"}){
      outword $smichut."y",        "ע,רבים,של/אני"; # possessives (kinu'im)
      outword $smichut."ינו",      "ע,רבים,של/אנחנו";
      outword $smichut."יך",       "ע,רבים,של/אתה";
      outword $smichut."yך",       "ע,רבים,של/את";
      outword $smichut_orig."יכם", "ע,רבים,של/אתם";
      outword $smichut_orig."יכן", "ע,רבים,של/אתן";
      outword $smichut."יו",       "ע,רבים,של/הוא";
      outword $smichut."יה",       "ע,רבים,של/היא";
      outword $smichut_orig."יהן", "ע,רבים,של/הן";
      outword $smichut_orig."יהם", "ע,רבים,של/הם";
      }
    }
  } elsif($opts{"ת"}){
    ############################# adjective ##################################
    $detail_prefix="";
    # preprocess the word the user has given, converting certain ktiv male
    # constructs into markers (w, y) that we can better work with later (see
    # comments in inword() about what it does).
    $word=inword($word);
    # A preprocessing rule special for adjectives: a final yud will always be
    # a chirik male, not some sort of consonant yud or another vowel. Together
    # with the iy post-transformation in outword, this makes שני - שנייה work
    # correctly. However, when the word ends with וי (and not ווי) we assume
    # this is shuruk followed by a consonant yud (for example, מצוי). In
    # words that do end in ווי and the וו is not a consonant we must use a
    # w explictly, (e.g. רווי should be written explictly as רwוי).
    if($word =~ m/([^aeiו]|וו)י$/o){
      substr($word,-1,1) = "iי";
    }

    my $xword=$word;
    if(substr($xword,-1,1) eq "ה"){
      # remove final "he" before adding the pluralization,
      # unless the שמור_ה option was given.
      if(!$opts{"שמור_ה"}){
        $xword=substr($xword,0,-1);
      }
    }

    if($opts{"עם"}){
      # For nationality adjectives (always adding in yud!), there is a seperate
      # plural for the people of that nationality (rather than other objects
      # from that country), with only ם added. There's also a country name,
      # and sometimes a female-person form too (נקבה_ה). We these here,
      # instead of seperately in extrawords, so that the country list can be
      # organized nicely at one place.
      if(exists($opts{"ארץ"})){
        outword $opts{"ארץ"}, "ע,פרטי,נ" if($opts{"ארץ"} ne "") # country name
      } elsif(substr($word,-3,3) eq "אiי"){
        outword substr($word,0,-3)."ה", "ע,פרטי,נ";  # country name
      } else {
        $country = $word;
        $country =~ s/i?י$//;
        $country =~ s/([כמנפצ])$/$fin{$1}/;
        outword $country, "ע,פרטי,נ"; # country name
      }
      outword $word."ם", "ע,רבים,ז"; # plural (people of that nationality)
      $opts{"נקבה_ת"}=1; # for enabling ת plural. adding ה plural is optional.
    }

    if(!exists($opts{"יחיד"})){
      outword $word,     "ת,יחיד,ז"; # masculin, singular
      outword $word."-", "ת,יחיד,ז,סמיכות"; # smichut (same as nifrad)
    } else {
      outword $opts{"יחיד"},     "ת,יחיד,ז"; # masculin, singular
      outword $opts{"יחיד"}."-", "ת,יחיד,ז,סמיכות"; # smichut (same as nifrad)
    }
    if($opts{"ם"}){
      # special case for adjectives like רשאי. Unlike the noun case where we
      # turn this option automatically for words ending with אי, here such a
      # default would not be useful because a lot of nouns ending with ה or א
      # correspond to adjectives ending with אי that this rule doesn't fit.
      outword $xword."ם",  "ת,רבים,ז"; # masculin, plural
      outword $xword."-",  "ת,רבים,ז,סמיכות"; # smichut
    } else {
      outword $xword."ים", "ת,רבים,ז"; # masculin, plural
      outword $xword."י-", "ת,רבים,ז,סמיכות"; # smichut
    }
    # feminine, singular:
    my $nekeva_implicit = !($opts{"נקבה_ת"} || $opts{"נקבה_ה"} ||
                            $opts{"נקבה_ית"} || $opts{"יחידה"});
    # by checking for final iי, we're basically checking for final י except
    # in final וי (see comment above on where we added the i)
    my $nekeva_t = $opts{"נקבה_ת"} ||
                   ($nekeva_implicit && substr($xword,-2,2) eq "iי");
    my $nekeva_h = $opts{"נקבה_ה"} ||
                   ($nekeva_implicit && !$nekeva_t);
    my $nekeva_it = $opts{"נקבה_ית"};
    if(exists($opts{"יחידה"})){
      my $yechida=$opts{"יחידה"};
      outword $yechida,     "ת,יחיד,נ";
      $yechida =~ s/ה$/ת/ if(!$opts{"שמור_ה"});
      outword $yechida."-", "ת,יחיד,נ,סמיכות";
    }
    if($nekeva_t){
      if(substr($word,-1,1) eq "ה" && !$opts{"שמור_ה"}){
        # This is a rare case, where an adjective ending with ה gets a ת
        # feminine form, and an extra yud needs to be added. For example
        # מופלה, מופלית.
        outword $xword."ית",  "ת,יחיד,נ";
        outword $xword."ית-", "ת,יחיד,נ,סמיכות"; # smichut (same as nifrad)
      } else {
        # note: we don't bother adding the vowel "e" before the ת because that
        # would only make a difference before a yud - and interestingly when
        # there *is* a yud, the vowel is dropped anyway!
        outword $xword."ת",   "ת,יחיד,נ";
        outword $xword."ת-",  "ת,יחיד,נ,סמיכות"; # smichut (same as nifrad)
      }
    }
    if($nekeva_h){
      outword $xword."aה",  "ת,יחיד,נ";
      outword $xword."aת-", "ת,יחיד,נ,סמיכות"; # smichut
    }
    if($nekeva_it){
      outword $xword."ית",  "ת,יחיד,נ";
      outword $xword."ית-", "ת,יחיד,נ,סמיכות"; # smichut
    }
    # Feminine, plural:
    # It stays the same, regardless of the singular for. The only exception
    # is the ית feminine, where the plural becomes יות. Note that there is
    # no "else" in the if below - because we need to support the cased that
    # one word has both types of plural (e.g., see אהבל).
    if($nekeva_h || $nekeva_t || $opts{"יחידה"}){
      outword $xword."ות",  "ת,רבים,נ"; # feminine, plural
      outword $xword."ות-", "ת,רבים,נ,סמיכות"; # smichut (same as nifrad)
    }
    if($nekeva_it){
      outword $xword."יות",  "ת,רבים,נ"; # feminine, plural
      outword $xword."יות-", "ת,רבים,נ,סמיכות"; # smichut (same as nifrad)
    }
  } else {
    die "word '".$word."' was not specified as noun, adjective or verb.";
  }
  outword "-------"
}
