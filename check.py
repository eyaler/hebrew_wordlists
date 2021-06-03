import os
import re

heb = 'א-ת'
lengths = {}
for file in sorted(os.listdir()):
    if not file.endswith('.txt'):
        continue
    with open(file, encoding='utf8') as f:
        text = f.read()
        print(file)

        bad = re.search('[^'+heb+'\'"\n]', text)
        assert not bad, bad # no non-hebrew chars

        lines = text.splitlines()

        empty = sum(not line for line in lines)
        assert not empty, empty # no empty lines

        lengths[file] = len(lines)
        len_set = len(set(lines))
        assert len(lines)==len_set, (len(lines), len_set) # no duplicate lines

        if 'append' not in file:
            sorted_lines = sorted(lines)
            assert lines==sorted_lines, (lines[:5], sorted_lines[:5]) # lines are sorted

        if 'with' in file:
            assert lengths[file] == lengths[file.replace('with', 'append')] == lengths[file.replace('with', 'no')]+lengths[file.replace('with', 'only')], (lengths[file], lengths[file.replace('with', 'append')], lengths[file.replace('with', 'no')] + lengths[file.replace('with', 'only')], lengths[file.replace('with', 'no')], lengths[file.replace('with', 'only')])

