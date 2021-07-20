import os
import csv

corpus_file = 'cc100.csv'
dict_files = ['all_no_fatverb.txt', 'all_no_fatverb_with_some_prefixes.txt', 'all_with_fatverb.txt', 'all_with_fatverb_with_some_prefixes.txt',
              'd:/data/text/heb/hspell/all_with_fatverb_with_all_prefixes.txt', 'd:/data/text/heb/hspell/all_with_fatverb_with_all_prefixes_with_he_hasheela.txt'] # note the last two files are 1.5GB and 2.2GB strong so they are not included in the repo

with open(corpus_file, encoding='utf8') as f:
    words_counts = list(csv.reader(f))

for file in dict_files:
    assert os.path.basename(file).startswith('all_'), file
    with open(file, encoding='utf8') as f:
        dict_words = set(f.read().splitlines())
    with open(os.path.splitext(os.path.basename(corpus_file))[0] + '_intersect_' + os.path.splitext(os.path.basename(file))[0][4:] +'.csv', 'w', encoding='utf8', newline='\r\n') as f:
        lines = ['%s,%s\n'%(word,cnt) for word, cnt in words_counts if word in dict_words]
        print('%s %d -> %d'%(file, len(dict_words), len(lines)))
        assert lines
        f.writelines(lines)
