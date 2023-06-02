import glob
import re


ALLOWED_WORD_TYPES = {
  None,
  'ז,נ,ע,יחיד',
  'ז,ע,יחיד',
  'נ,ע,יחיד',
  'ע',
  'ע,ז',
  'ע,ז,יחיד',
  'ע,ז,יחיד,פרטי',
  'ע,ז,פרטי',
  'ע,יחיד',
  'ע,נ,יחיד',
  'ע,נ,יחיד,פרטי',
  'ע,נ,פרטי',
  'ע,פרטי',
  'ע,פרטי,ז',
  'ע,פרטי,נ',
  'פ,מקור',
  'פ,עבר,הוא',
  'ת,ז,יחיד',
  'ת,יחיד',
  'ת,יחיד,ז',
}


def process_lines(f):
  for line in f:
    line = line.strip()
    if not line: continue
    if line[0] == '#': continue
    if line[:3] == '---': continue
    space_pos = line.find(' ')
    if space_pos == -1:
      word = line
      word_type = None
    else:
      word = line[:line.find(' ')]
      word_type = line[line.find(' ') + 1:]
    if word[0] in ('B', 'L'):
      word = word[1:]  # Prefix for some verbs.
    if word_type in ALLOWED_WORD_TYPES:
      yield word


def main():
  words = set()

  # To get a complete word list, this should be run
  # on all .hif files in hspell-1.4.
  for fname in glob.glob('*.hif'):
    print(f'Reading file {fname}...')
    with open(fname, 'rt', encoding='cp1255') as f:
      words.update(process_lines(f))

  print(f'Found {len(words)} valid base word forms')

  with open('all_base_word_forms.txt', 'wb') as f:
    for word in sorted(words):
      f.write(f'{word}\n'.encode('utf8'))


if __name__ == '__main__':
  main()
