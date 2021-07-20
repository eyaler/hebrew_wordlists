import os
import re

heb = 'א-ת'
lengths = {}
head_mid_tails = set()
head_mid_tail_len = 1
base_name = None
palin_anad_files = sorted(os.path.join('palindromes_anadromes', file) for file in os.listdir('palindromes_anadromes'))
for file in sorted(os.listdir()) + palin_anad_files:
    ext = os.path.splitext(file)[1]
    if not ext in ['.txt','.csv'] or 'rejected' in file or file=='requirements.txt':
        continue
    with open(file, encoding='utf8', newline='') as f:
        text = f.read()
    sep = '\r\n' if ext=='.csv' else '\n'
    lines = text.split(sep)[:-1]
    if ext=='.csv':
        counts = [line.split(',')[1] for line in lines]
        lines = [line.split(',')[0] for line in lines]
    print(file, len(lines))
    assert lines # file is not empty
    assert text.endswith(sep), text.split(sep)[-2:] # wrong EOL or missing last empty line

    bad = re.search(r'[\d]', file.replace('cc100',''))
    assert not bad, bad  # no digits which may signify temporary files

    for line in lines:
        if 'bigrams' in file:
            assert line.count(' ')==1, (line, line.count(' '))
        extra = ''
        if 'prefixes' in file or 'family' in file or 'place' in file:
            extra += '-'
        if 'bigrams' in file or 'family' in file or 'place' in file:
            extra += ' '
        bad = re.search(r'(^|\W)\'|^["\s-]|["\s-]$|["\s-]{2,}|[^\'"' + heb + extra +']', line)
        assert not bad, (bad, line)  # no non-hebrew chars

    name, ext = os.path.splitext(file)

    if name.endswith('fatverb'):
        base_name = name
        base_set = set(lines)
    elif base_name and name.startswith(base_name) and not name.startswith(base_name+'_only_'):
        assert set(lines)>base_set, (len(lines), len(base_set), len(base_set-set(lines))) # make sure prefix files also include non-prefixed words

    if ext=='.csv':
        for line in counts:
            bad = re.search('r[^\d]', line)
            assert not bad, (bad, line) # only numbers for counts

    head_mid_tail = tuple(lines[:head_mid_tail_len] + lines[len(lines)//2-head_mid_tail_len//2:len(lines)//2-head_mid_tail_len//2+head_mid_tail_len] + lines[-head_mid_tail_len:])
    assert head_mid_tail not in head_mid_tails
    head_mid_tails.add(head_mid_tail) # check that no files are identical

    empty = sum(not line.strip() for line in lines)
    assert not empty, empty  # no empty lines

    lengths[file] = len(lines)
    len_set = len(set(lines))
    assert len(lines) == len_set, (len(lines), len_set)  # no duplicate lines

    if ext=='.csv':
        lines_counts = list(zip(lines,counts))
        sorted_lines = sorted(lines_counts, key=lambda x:(-int(x[1]),x[0]))
        assert lines_counts == sorted_lines, (lines_counts[:5], sorted_lines[:5])  # lines are sorted
    elif 'append' not in file and 'intersect' not in file:
        sorted_lines = sorted(lines)
        assert lines == sorted_lines, (lines[:5], sorted_lines[:5])  # lines are sorted

    if ext!='.csv' and 'palin' not in file and 'anad' not in file:

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
    if not os.path.splitext(file)[1] in ['.txt','.csv']:
        continue
    try:
        open(file, 'a', encoding='utf8')
    except Exception:
        continue
    assert False, file+' is not readonly'
