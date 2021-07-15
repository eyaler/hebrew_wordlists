import os
import re

heb = 'א-ת'
lengths = {}
head_mid_tails = set()
head_mid_tail_len = 1
palin_anad_files = [os.path.join('palin_and_anad', file) for file in os.listdir('palin_and_anad')]
for file in sorted(os.listdir()+palin_anad_files):
    if not file.endswith(('.txt','.csv')) or 'rejected' in file:
        continue
    with open(file, encoding='utf8') as f:
        text = f.read()
        lines = text.split('\n')
        if file.endswith('csv'):
            counts = [line.split(',')[1] for line in lines]
            lines = [line.split(',')[0] for line in lines]
        print(file, len(lines))

        bad = re.search(r'[\d]', file.replace('cc100',''))
        assert not bad, bad  # no digits which may signify temporary files

        for line in lines:
            bad = re.search('[^' + heb + '\'"'+('-' if 'prefixes' in file else '')+(' -' if 'family' in file or 'settlement' in file else '')+']', line.rstrip('\n'))
            assert not bad, bad  # no non-hebrew chars

        if file.endswith('csv'):
            for line in counts:
                bad = re.search('r[^\d]', line.rstrip('\n'))
                assert not bad, bad  # only numbers for counts

        head_mid_tail = tuple(lines[:head_mid_tail_len] + lines[len(lines)//2-head_mid_tail_len//2:len(lines)//2-head_mid_tail_len//2+head_mid_tail_len] + lines[-head_mid_tail_len:])
        assert head_mid_tail not in head_mid_tails
        head_mid_tails.add(head_mid_tail) # check that no files are identical

        empty = sum(not line.strip() for line in lines)
        assert not empty, empty  # no empty lines

        lengths[file] = len(lines)
        len_set = len(set(lines))
        assert len(lines) == len_set, (len(lines), len_set)  # no duplicate lines

        if file.endswith('csv'):
            lines_counts = list(zip(lines,counts))
            sorted_lines = sorted(lines_counts, key=lambda x:(-int(x[1]),x[0]))
            assert lines_counts == sorted_lines, (lines_counts[:5], sorted_lines[:5])  # lines are sorted
        elif 'append' not in file:
            sorted_lines = sorted(lines)
            assert lines == sorted_lines, (lines[:5], sorted_lines[:5])  # lines are sorted

        if 'palin' not in file and 'anad' not in file:

            if 'with_fatverb' in file and 'prefixes' not in file:
                assert lengths[file] == lengths[file.replace('with_fatverb', 'append_fatverb')], (
                    lengths[file], lengths[file.replace('with_fatverb', 'append_fatverb')])
                assert lengths[file] == lengths[file.replace('with_fatverb', 'no_fatverb')] + lengths[
                    file.replace('with_fatverb', 'only_fatverb')], (lengths[file],
                                                                    lengths[file.replace('with_fatverb', 'no_fatverb')] +
                                                                    lengths[file.replace('with_fatverb', 'only_fatverb')],
                                                                    lengths[file.replace('with_fatverb', 'no_fatverb')],
                                                                    lengths[file.replace('with_fatverb', 'only_fatverb')])

            if 'with_some_prefixes' in file:
                assert lengths[file] == lengths[file.replace('with_some_prefixes', 'append_some_prefixes')], (
                    lengths[file], lengths[file.replace('with_some_prefixes', 'append_some_prefixes')])

assert lengths['all_with_fatverb_append_some_prefixes.txt'] == lengths[
    'all_append_fatverb_after_append_some_prefixes.txt'] == lengths[
           'all_append_fatverb_before_append_some_prefixes.txt'], (
    lengths['all_with_fatverb_append_some_prefixes.txt'], lengths['all_append_fatverb_after_append_some_prefixes.txt'],
    lengths['all_append_fatverb_before_append_some_prefixes.txt'])

#check readonly
for file in sorted(os.listdir()):
    if not file.endswith(('.txt','.csv')):
        continue
    try:
        open(file, 'a', encoding='utf8')
    except Exception:
        continue
    assert False, file+' is not readonly'
