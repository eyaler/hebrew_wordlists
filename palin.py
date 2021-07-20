import os
from pygtrie import CharTrie
from time import time

files = ['bible.txt', 'israeli_first_names.txt', 'cc100_intersect_with_fatverb.csv', 'cc100_intersect_with_fatverb_with_some_prefixes.csv',
         'cc100_intersect_with_fatverb_with_all_prefixes.csv', 'cc100_intersect_with_fatverb_with_all_prefixes_with_he_hasheela.csv',
         'cc100.csv', 'all_with_fatverb.txt', 'all_with_fatverb_with_some_prefixes.txt',
         'd:/data/text/heb/hspell/all_with_fatverb_with_all_prefixes.txt', 'd:/data/text/heb/hspell/all_with_fatverb_with_all_prefixes_with_he_hasheela.txt'] # note the last two files are 1.5GB and 2.2GB strong so they are not included in the repo

min_cnt = 1
test_bigrams = False

def minify(text):
    return text.strip().replace('"','').replace("'",'').replace('-','').replace(' ','')

os.makedirs('palindromes_anadromes', exist_ok=True)
grand_start = time()
with open(os.path.join('palindromes_anadromes','palin_stats.log'), 'w', encoding='utf8') as log:
    for file in files:
        start = time()
        palin = []
        anad = []
        palin_anad = []
        bigrams = []
        trie = CharTrie()
        do_bigrams = file!='cc100.csv' and not file.startswith('d:/data')
        with open(file, encoding='utf8') as f:
            lines = f.readlines()
        name, ext = os.path.splitext(os.path.basename(file))
        if ext == '.csv':
            lines = [line.split(',')[0]+'\n' for line in lines if int(line.split(',')[1])>=min_cnt]
        print('%s %d' % (file, len(lines)))
        log.write('%s %d\n' % (file, len(lines)))
        log.flush()
        clean_lines = [minify(line).replace('ך','כ').replace('ם','מ').replace('ן','נ').replace('ף','פ').replace('ץ','צ') for line in lines]
        clean_lines_set = set(clean_lines)
        for line, clean in zip(lines, clean_lines):
            if clean==clean[::-1]:
                palin.append(line)
                palin_anad.append(line)
            elif clean[::-1] in clean_lines_set:
                anad.append(line)
                palin_anad.append(line)
            if do_bigrams:
                if clean[::-1] not in trie:
                    trie[clean[::-1]] = []
                trie[clean[::-1]].append(line)
        if do_bigrams:
            for line, clean in zip(lines, clean_lines):
                for i in range(-len(clean)+1,0):
                    if clean[i:]==clean[i:][::-1]:
                        break
                if trie.has_node(clean[:i]):
                    for rev, lines2 in trie.iteritems(clean[:i]):
                        if clean+rev[::-1]==rev+clean[::-1]:
                            for line2 in lines2:
                                bigrams.append(line.strip()+' '+line2)

            if test_bigrams:
                from collections import defaultdict
                rev = defaultdict(list)
                minlen = len(min(clean_lines, key=len))
                for line, clean in zip(lines, clean_lines):
                    rev[clean[-1:-1-minlen:-1]].append((line, clean))
                concat = defaultdict(list)
                for line, clean in zip(lines, clean_lines):
                    for line2, clean2 in rev[clean[:minlen]]:
                        if clean+clean2==clean2[::-1]+clean[::-1]:
                            concat[clean+clean2].append(line.strip()+' '+line2)
                slow_bigrams = {bigram for concat_clean,concat_bigrams in concat.items() for bigram in concat_bigrams if concat_clean[::-1] in concat}
                assert set(bigrams) == slow_bigrams, (len(bigrams), len(slow_bigrams), sorted(bigrams)[:5], sorted(slow_bigrams)[:5])

        palin = sorted(list(dict.fromkeys(palin)))
        anad = sorted(list(dict.fromkeys(anad)))
        palin_anad = sorted(list(dict.fromkeys(palin_anad)))

        maxp = max(len(minify(x)) for x in palin)
        maxa = max(len(minify(x)) for x in anad)

        output = [('longest palindromes with %d chars:'%maxp, [x.strip() for x in palin if len(minify(x))==maxp])]
        output.append(('longest proper anadromes with %d chars:'%maxa,[x.strip() for x in anad if len(minify(x))==maxa]))

        if do_bigrams:
            bigrams = sorted(list(dict.fromkeys(bigrams)))
            maxb = max(len(minify(x)) for x in bigrams)
            output.append(('longest palidromic bigram with %d chars:'%maxb,[x.strip() for x in bigrams if len(minify(x)) == maxb]))

        output.append(('palin=%d anad=%d both=%d bigrams=%d took=%.1f min\n'%(len(palin),len(anad),len(palin)+len(anad),len(bigrams),(time()-start)/60),))
        for line in output:
            print(*line)
            log.write(' '.join(str(s) for s in line)+'\n')
            log.flush()
        with open(os.path.join('palindromes_anadromes','palindromes_%s.txt')%name,'w',encoding='utf8', newline='\n') as f:
            f.writelines(palin)
        with open(os.path.join('palindromes_anadromes','anadromes_%s.txt')%name,'w',encoding='utf8', newline='\n') as f:
            f.writelines(anad)
        with open(os.path.join('palindromes_anadromes','palin_and_anad_%s.txt')%name,'w',encoding='utf8', newline='\n') as f:
            f.writelines(palin_anad)
        if do_bigrams:
            with open(os.path.join('palindromes_anadromes','palin_bigrams_%s.txt')%name,'w',encoding='utf8', newline='\n') as f:
                f.writelines(bigrams)
print('took %d min'%((time()-grand_start)/60))