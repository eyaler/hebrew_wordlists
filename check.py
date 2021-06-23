import os
import re

heb = 'א-ת'
lengths = {}
head_mid_tails = set()
head_mid_tail_len = 1
for file in sorted(os.listdir()):
    if not file.endswith('.txt'):
        continue
    with open(file, encoding='utf8') as f:
        text = f.read()
        print(file)

        bad = re.search(r'[\d]', file)
        assert not bad, bad  # no digits which may signify temporary files

        bad = re.search('[^' + heb + '\'"\n'+('-' if 'prefixes' in file else '')+']', text)
        assert not bad, bad  # no non-hebrew chars

        lines = text.split('\n')
        head_mid_tails.add(tuple(lines[:head_mid_tail_len] + lines[len(lines)//2-head_mid_tail_len//2:len(lines)//2-head_mid_tail_len//2+head_mid_tail_len] + lines[-head_mid_tail_len:]))

        empty = sum(not line.strip() for line in lines)
        assert not empty, empty  # no empty lines

        lengths[file] = len(lines)
        len_set = len(set(lines))
        assert len(lines) == len_set, (len(lines), len_set)  # no duplicate lines

        if 'append' not in file:
            sorted_lines = sorted(lines)
            assert lines == sorted_lines, (lines[:5], sorted_lines[:5])  # lines are sorted

        if 'with_fatverb' in file and 'prefixes' not in file:
            assert lengths[file] == lengths[file.replace('with_fatverb', 'append_fatverb')], (
                lengths[file], lengths[file.replace('with_fatverb', 'append_fatverb')])
            assert lengths[file] == lengths[file.replace('with_fatverb', 'no_fatverb')] + lengths[
                file.replace('with_fatverb', 'only_fatverb')], (lengths[file],
                                                                lengths[file.replace('with_fatverb', 'no_fatverb')] +
                                                                lengths[file.replace('with_fatverb', 'only_fatverb')],
                                                                lengths[file.replace('with_fatverb', 'no_fatverb')],
                                                                lengths[file.replace('with_fatverb', 'only_fatverb')])

        if 'with_some_prefixes' in file and 'palin' not in file and 'anad' not in file:
            assert lengths[file] == lengths[file.replace('with_some_prefixes', 'append_some_prefixes')], (
                lengths[file], lengths[file.replace('with_some_prefixes', 'append_some_prefixes')])

assert lengths['all_with_fatverb_append_some_prefixes.txt'] == lengths[
    'all_append_fatverb_after_append_some_prefixes.txt'] == lengths[
           'all_append_fatverb_before_append_some_prefixes.txt'], (
    lengths['all_with_fatverb_append_some_prefixes.txt'], lengths['all_append_fatverb_after_append_some_prefixes.txt'],
    lengths['all_append_fatverb_before_append_some_prefixes.txt'])

assert len(head_mid_tails) == len(lengths), (len(head_mid_tails), len(lengths)) # check that no files are identical

#check readonly
for file in sorted(os.listdir()):
    if not file.endswith('.txt'):
        continue
    try:
        open(file, 'a', encoding='utf8')
    except Exception:
        continue
    assert False, file+' is not readonly'
