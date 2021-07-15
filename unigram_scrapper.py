import os
import sys
sys.path.insert(1, os.path.join('..', 'hebrew_tokenizer'))
from hebrew_tokenizer import HebTokenizer
from collections import Counter

bad_str = []
max_line_chars_allowed = 0

#file = 'd:/data/text/heb/oscar_he_dedup.txt' # https://oscar-public.huma-num.fr/shuff-dedup/he
#bad_str = ['גוגל מפות', '|']
#max_line_chars_allowed = 4000

file = 'd:/data/text/heb/cc100_he.txt' # http://data.statmt.org/cc-100/

corpus = file.rsplit('/', 1)[-1].split('_')[0]
heb_tokenizer = HebTokenizer()
words = Counter()
i = 0
skip_lines = 0
skip_words = Counter()
cnt_counter = Counter()
len_counter = Counter()
max_line_chars = 0
max_line_words = 0
max_line_words_counter = Counter()
lines_above_max_chars_allowed = 0
with open(file, encoding='utf8') as f, open('d:/data/text/heb/%s_rejected_lines.txt'%corpus, 'w', encoding='utf8') as fbad:
    for line in f:
        i+=1
        if i%1000000==0:
            print(i)
        line_words = heb_tokenizer.get_words(line)
        if len(line)>max_line_chars:
            max_line_chars = len(line)
            max_line_chars_ind = i
        if len(line_words)>max_line_words:
            max_line_words = len(line_words)
            max_line_words_ind = i
            max_line_words_counter = Counter(line_words)
        label = str(len(line_words)) if len(line_words) < 10 else '10+'
        len_counter[label]+=1
        if max_line_chars_allowed and len(line)>max_line_chars_allowed or any(bad in line for bad in bad_str):
            if max_line_chars_allowed and len(line)>max_line_chars_allowed:
                lines_above_max_chars_allowed += 1
            skip_lines += 1
            skip_words.update(line_words)
            fbad.write(line)
        else:
            words.update(line_words)

sum_cnt = 0
with open('%s.csv'%corpus, 'w', encoding='utf8') as f:
    for i, (word, cnt) in enumerate(sorted(words.items(), key=lambda x: (-x[1], x[0]))):
        sum_cnt += cnt
        label = str(cnt) if cnt<10 else '10+'
        cnt_counter[label] += 1
        f.write('%s,%d%s'%(word, cnt, '\n' if i<len(words) else ''))

skip_cnt = 0
with open('%s_rejected_words.csv'%corpus, 'w', encoding='utf8') as csv:
    for word, cnt in sorted(skip_words.items(), key=lambda x: (-x[1], x[0])):
        csv.write('%s,%d\n'%(word, cnt))
        skip_cnt += cnt

total_words = skip_cnt+sum_cnt
print('len_counter:', len_counter.most_common())
print('cnt_counter:', cnt_counter.most_common())
print('max_line_words_counter:', max_line_words_counter.most_common(10))
print('corpus=%s total_lines=%d total_words=%d used_lines=%d (%.2f) used_words=%d (%.2f) unique_words=%d skipped_lines=%d (%.2f) skipped_words=%d (%.2f) lines_above_max_chars_allowed=%d max_line_chars=%d (i=%d) max_line_words=%d (i=%d)'%
      (corpus, i, total_words, i-skip_lines, (i-skip_lines)/i, sum_cnt, sum_cnt/total_words, len(words), skip_lines, skip_lines/i, skip_cnt, skip_cnt/total_words, lines_above_max_chars_allowed, max_line_chars, max_line_chars_ind, max_line_words, max_line_words_ind))

'''
cc100:
len_counter: [('10+', 79051694), ('2', 24775417), ('3', 21289633), ('4', 16996423), ('0', 13244077), ('5', 13205121), ('6', 11153959), ('7', 9513575), ('8', 8386870), ('9', 6987521), ('1', 2938629)]
cnt_counter: [('1', 1955668), ('10+', 1060950), ('2', 600957), ('3', 298162), ('4', 190193), ('5', 132969), ('6', 101706), ('7', 91693), ('8', 66981), ('9', 55158)]
corpus=cc100 total_lines=207542919 total_words=3140829404 used_lines=207542919 (1.00) used_words=3140829404 (1.00) unique_words=4554437 skipped_lines=0 (0.00) skipped_words=0 (0.00)

cc100 skipping 'גוגל מפות', '|' and len>4000:
len_counter: [('10+', 79051694), ('2', 24775417), ('3', 21289633), ('4', 16996423), ('0', 13244077), ('5', 13205121), ('6', 11153959), ('7', 9513575), ('8', 8386870), ('9', 6987521), ('1', 2938629)]
cnt_counter: [('1', 1954447), ('10+', 1060415), ('2', 600708), ('3', 297955), ('4', 190022), ('5', 132927), ('6', 101692), ('7', 91652), ('8', 66956), ('9', 55133)]
max_line_words_counter: [('את', 100), ('של', 55), ('לא', 40), ('או', 24), ('מה', 17), ('עם', 16), ('על', 15), ('בין', 13), ('ללא', 12), ('לך', 11)]
corpus=cc100 total_lines=207542919 total_words=3140829404 used_lines=207255007 (1.00) used_words=3134313276 (1.00) unique_words=4551907 skipped_lines=287912 (0.00) skipped_words=6516128 (0.00) lines_above_max_chars_allowed=1722 max_line_chars=15608 (i=90632729) max_line_words=2678 (i=90632729)

oscar skipping 'גוגל מפות' and len>4000:
len_counter: [('10+', 20641172), ('0', 69689), ('9', 36517), ('8', 31133), ('7', 26444), ('6', 20705), ('5', 11660), ('4', 8038), ('2', 5758), ('1', 3420), ('3', 3380)]
cnt_counter: [('1', 1357968), ('10+', 708344), ('2', 388616), ('3', 197632), ('4', 127616), ('5', 88030), ('6', 68057), ('7', 53840), ('8', 43818), ('9', 36641)]
max_line_words_counter: [('קקי', 16780), ('אוזון', 16780), ('בולבולים', 12527)]
corpus=oscar total_lines=20857916 total_words=948706943 used_lines=19483761 (0.93) used_words=874066233 (0.92) unique_words=3070562 skipped_lines=1374155 (0.07) skipped_words=74640710 (0.08) max_line_chars=490572 (7334090) max_line_words=46087 (17984717)

oscar skipping 'גוגל מפות', '|' and len>4000:
len_counter: [('10+', 20641172), ('0', 69689), ('9', 36517), ('8', 31133), ('7', 26444), ('6', 20705), ('5', 11660), ('4', 8038), ('2', 5758), ('1', 3420), ('3', 3380)]
cnt_counter: [('1', 1353895), ('10+', 706412), ('2', 387259), ('3', 196966), ('4', 127201), ('5', 87697), ('6', 67831), ('7', 53780), ('8', 43636), ('9', 36564)]
max_line_words_counter: [('קקי', 16780), ('אוזון', 16780), ('בולבולים', 12527)]
corpus=oscar total_lines=20857916 total_words=948706943 used_lines=19275390 (0.92) used_words=868826656 (0.92) unique_words=3061241 skipped_lines=1582526 (0.08) skipped_words=79880287 (0.08) max_line_chars=490572 (7334090) max_line_words=46087 (17984717)
'''