<div dir="rtl">
<h2>הצהרה ואזהרה</h2>

אין מילון מלבד Hspell ונדב הראל ודן קניגסברג הם נביאי האמת

עם זאת, ישנן מספר לקונות שכדאי לקחת בחשבון:

- המילון לא עודכן עם מילים חדשות מאז 2017

- המילון לא עודכן ל[כללי הכתיב המלא מ-2017](https://hebrew-academy.org.il/topic/hahlatot/missingvocalizationspelling). בפרט:

  - מילים להן התווספה ו כמו: אונייה, חוכמה, למוחרת, צוהריים, קודקוד, הקורבן, קורבני, תוכנות, תוכניתי, האומנותי (איך אפשר לקבוע להיפגש במקום שאינו במילון??)

  - מילים להן התווספה י כמו: אידיאל (לאידיאל!), איטי/ות, אימו/ך, איתה/ו/ך/ם, אמיתי/ת, בריכה, הינה/ו/ך/ם/ני, ליבי/ך/ם, מגינים, מייד, המיידי, מקילות, סיסמה, עימו/י/ך, 
עיזים, לעיתים, פירות, ריאלי, השיער, שיערות, תיאטרון

  - הבעיה כמובן היא להכליל לנטיות השונות ולתחיליות שונות

  - מאידך כן מופיעות הצורות הישנות שכיום אינן עוד תקניות 

- חסרות נטיות של מילים מסויימות בבניין [נתפעל](https://hebrew-academy.org.il/2018/07/24/התבקשנו-או-נתבקשנו-על-התפעל-ונתפעל) כמו: נזדקקתי, נתבדיתי, נתפוגגתי

- חסרות צורות מאורכות שונות כמו: אשובה, הספיקותי, נזוזה

<h2>מיון</h2>
	
קבצי txt. שאין בשמם "append", ממוינים לפי סדר לקסיקוגרפי הוא סדר המיון בפייתון, ב-++notepad, ובלינוקס עם LC_ALL=C

קבצי csv. ממוינים לפי הספירות

<h2>סופיות מושא ישיר</h2>
	
הרשימות שבשמן "fatverb" כוללות את כינויי המושא החבורים

<h2>תחיליות</h2>
	
רשימות המילים שכוללות מילים עם תחיליות מוגבלות לתחיליות הבאות בלבד:

- מ ש ה ו כ ל ב כש

- השילובים של הנ"ל עם ו החיבור: ומ וש וה וכ ול וב וכש

- השילובים של הנ"ל עם ה הידיעה: מה שה כשה ומה ושה וכשה

- היכן שמוכפלת ו' בראש מילה מופיעה גם חלופה עם מקף במקום ההכפלה

- לא נעשה שימוש ב-ה השאלה

ניתן למצוא את הקוד לנ"ל בתיקייה modified_hspell_src.
כיוון ש-unmunch לא עובד טוב עם UTF8 (הוא מפספס חוקים), השתמשתי ב-[dodic.pl](https://github.com/hunspell/hunspell/files/5696469/dodic.pl.txt)

<h2>מילים פלינדרומיות ואנדרומיות</h2>
	
רשימות הפלינדרומים והאנדרומים מופיעות גם בגרסאות הכוללות את כל התחיליות של Hspell בנוסף לאופציה של ה השאלה

עדיין אלו אינן ממצות את כל האפשרויות בעברית - השוו ל-[CFG של שלמה יונה](http://www.ivrix.org.il/doc/iscol04.pdf) שממומש ב-genprefixes.pl רק בצורה חלקית

בעזרת הרשימות האלו מצאתי בנוסף לשיאים הקודמים שלי בני 11 האותיות: מכיתומותיכם ו- מכהפוכופהכם

גם את: הלכשמכמשכלה, ולכשמכמשכלו, ושמשמכמשמשו, שכשכמהמכשכש; שאני לא בטוח מה משמעותם אבל היי Hspell מאשר!

וכן שיאים אנדרומיים גם הם באורך 11: הלכשמכמשכלו-ולכשמכמשכלה, הלכשמכמשמשו-ושמשמכמשכלה, ולכשמכמשמשו-ושמשמכמשכלו

צירופים פלינדרומיים 

<h2>רשימות נוספות</h2>

- [CC-100](http://data.statmt.org/cc-100) - קורפוס שנחצב מהרשת בן 33GB. המילים חולצו באמצעות [טוקנייזר שלי](https://github.com/eyaler/hebrew_tokenizer) שזורק בן השאר ראשי תיבות וקיצורים. כולל ספירות וממוין לפיהן

- חיתוכים בין CC-100 ו-Hspell.

- [שמות פרטיים בישראל](https://data.gov.il/dataset/firs-name) (israeli_first_names.txt) מ-2020. מפורק לשמות יחידים. משום מה נעדר השם השני שלי... הערה: [כאן](https://www.cbs.gov.il/he/publications/LochutTlushim/2020/שמות-פרטיים.xlsx) יש שמות לא מפורקים מ-2019 בחלוקות וספירות לפי דת, מגדר ושנת לידה 

- [שמות משפחה בישראל](https://www.cbs.gov.il/he/mediarelease/Pages/2019/שמות-משפחה-בישראל-2017.aspx
) (israeli_family_names.csv, israeli_jewish_family_names.csv) מ-2017. כולם או רק יהודים. מכיל רווחים ומקפים. עם ספירת נפשות וממוין לפיהן 

- [שמות ישובים בישראל](https://data.gov.il/dataset/citiesandsettelments) (israeli_place_names.txt) מ-7/2021. כולל נפות ומועצות. מכיל רווחים ומקפים  

- [תנ"ך](https://github.com/openscriptures/morphhb/blob/master/WlcWordList/Words.csv) (bible.txt)

</div>

<h2>Todo</h2>

(p1) Hierarchical combinations of CC-100 and Hspell

(p1) Filter CC-100 to reduce legal and commercial content (or consider other corpuses)

(p2) Rerank palindrome/anadrome lists by (joint) counts

(p2) Extract bigrams and counts from CC100    

(p3) Split part-of-speech lists by base forms, gender and plurality

(p3) Use the above splits to find palindromic bigrams of sentence form (e.g. noun+adj, noun+verb) and matching gender and plurality (and perhaps definiteness) 

(p4) Extract my own Bible wordlist

(p4) Split שמות-פרטיים.xlsx by gender and Jewishness

(p5) Palindrome/anadrome lists using ALL possible Hebrew prefixes

(p5) Allow CC-100 words to include acronyms and abbreviations

<h2>Remarks</h2>

This file was written using [Markdown Editor](https://jbt.github.io/markdown-editor), the only live editor I found that respects html dir=rtl tags, while retaining markdown semantics inside the div 

<h2>-------------------- Origianl Hspell README below this line --------------------</h2>

This is version 1.4 of Hspell, the free Hebrew spellchecker and morphology
engine.

You can get Hspell from:
	http://hspell.ivrix.org.il/

Hspell was written by Nadav Har'El and Dan Kenigsberg:
	nyh    @ math.technion.ac.il
	danken @   cs.technion.ac.il

Hspell is free software, released under the GNU Affero General Public License
(AGPL) version 3. Note that not only the programs in the distribution, but
also the dictionary files and the generated word lists, are licensed under
the AGPL.
There is no warranty of any kind for the contents of this distribution.
See the LICENSE file for more information and the exact license terms.

The rest of this README file explains Hspell's spelling standard (niqqud-less),
a bit about the technology behind Hspell, how to use the "hspell" program
(but see the manual page for more current information), and lists a few future
directions. See the separate INSTALL file for instructions on how to install
Hspell.


About Hspell's spelling standard
--------------------------------

Hspell was designed to be 100% and strictly compliant with the official
niqqud-less spelling rules ("Ha-ktiv Khasar Ha-niqqud", colloquially known as
"Ktiv Male", or "plene spelling" in English), published by the Academy of
the Hebrew Language. This is both an advantage and a disadvantage, depending
on your viewpoint. It's an advantage because it encourages a *correct* and
consistent spelling style throughout your writing. It is a disadvantage,
because a few of the Academia's official spelling decisions are relatively
unknown to the general public.

Users of Hspell (and all Hebrew writers, for that matter) are encouraged to
read the Academia's official niqqud-less spelling rules (which are printed at
the end of most modern Hebrew dictionaries), and to refer to Hebrew
dictionaries which use the niqqud-less spelling (such as Millon Ha-hove or
Rav Milim). We also provide in docs/niqqudless.odt a document (in Hebrew)
which describes in detail Hspell's spelling standard, and why certain words
are spelled the way they are.


The technology behind Hspell
----------------------------

The "hspell" program itself is mostly a simple (but efficient) program
that checks input words against a long list of valid words. The real "brains"
behind it are the word lists (lexicon) provided by the Hspell project.

In order for it to be completely free of other people's copyright restrictions,
the Hspell project is a clean-room implementation, not based on other
companies' word lists, on other companies' spell checkers, or on copying of
printed dictionaries. The word list is also not based on automatic scanning
of available Hebrew documents (such as online newspapers), because there is
no way to guarantee that such a list will be correct, complete, or consistent
with regard to spelling rules.

Instead, our idea was to write programs which know how to correctly inflect
Hebrew nouns and conjugate Hebrew verbs. The inputs to these programs are
lists of noun stems and of verb roots, plus hints needed for the correct
inflection when these cannot be figured out automatically. These input files
are obviously an important part of the Hspell project. The "word list
generators" (written in Perl, and are also part of the Hspell project) then
create the complete word-list for use by the spellchecking program, hspell.
The generated lists are useful for much more than spellchecking, by the
way - see more on that below ("the future").

Although we wrote all of Hspell's code ourselves, we are truly indebted to
the old-style "open source" pioneers - people who wrote books about the
knowledge they developed, instead of hiding it in proprietary software.
For the correct noun inflections, Dr. Shaul Barkali's "The Complete Noun Book"
has been a great help. Prof. Uzzi Ornan's booklet "Verb Conjugation in Flow
Charts" has been instrumental in the implementation of verb conjugation,
and Barkali's "The Complete Verb Book" was used too.

During our work we have extensively used a number of Hebrew dictionaries,
including Even Shoshan, Millon Ha-hove and Rav-Milim, to ensure the correctness
of certain words. Various Hebrew newspapers and books, both printed and online,
were used for inspiration and for finding words we still do not recognize.
We wish to thank Cilla Tuviana and Dr. Zvi Har'El for their assistance with
some grammatical questions.


Using hspell
------------

After unpacking the distribution and running "configure", "make" and
"make install" (see the INSTALL file for more information), the hspell
executable is installed (by default) in /usr/local/bin, and the dictionary
files are in /usr/local/share/hspell.

The "hspell" program can be used on any sort of text file containing Hebrew
and potentially non-Hebrew characters which it ignores. For example, it
works well on Hebrew text files, TeX/LaTeX files, and HTML. Running

	hspell filename

Will check the spelling in filename and will output the list of incorrect
words (just like the old-fashioned UNIX "spell" program did). If run without
a file parameter, hspell reads from its standard input.

In the current release, hspell expects ISO-8859-8-encoded files. If files
using a different encoding (e.g., UTF8) are to be checked, they must be
converted first to ISO-8859-8 (e.g., see iconv(1), recode(1)).

If the "-c" option is given, hspell will suggest corrections for misspelled
words, whenever it can find such corrections. The correction mechanism in this
release is especially good at finding corrections for incorrect niqqud-less
spellings, with missing or extra 'immot-qri'a.

The "-l" (verbose) option will explain for each correct word why it was
recognized, if Hspell was built with the "linginfo" optional feature enabled
(a morphological analysis is shown, i.e., fully describe all possible ways to
read the given word as an inflected word with optional prefixes).

Because hspell's output (naturally) is "logical-order", it is normally
useful to pipe it to bidiv or rev before viewing. For example

	hspell -c filename | bidiv | less

Another convenient alternative is to run hspell on a BiDi-enabled terminal.

Instead of using the hspell program described above, users can also use
Hspell's lexicon through one of the popular multi-lingual spell-checkers,
aspell and hunspell. See the INSTALL file for more information on building
these dictionaries.


How *you* can help
------------------

By now, Hspell is fairly mature, and its lexicon of over 24,000 base words
is fairly comprehensive, similar in breadth to some printed dictionaries.
Careful attention has also been given to its accuracy, and its conformance
with the spelling rules of the Academy of the Hebrew Language.

Nevertheless, Hspell does not, and probably never will, cover all of modern
Hebrew language. Also, undoubtedly, it may contain some errors as well.
If you find such omissions or errors, please let us know.

Before reporting such omissions or errors, please try to verify that the word
you are proposing is indeed correctly spelled: Please refer to modern
dictionaries. Please also look at doc/niqqudless.odt - the word you are
proposing might actually be a known mispelling which we discuss in that
document.
