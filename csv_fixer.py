import csv

files = ['israeli_family_names.csv', 'israeli_jewish_family_names.csv']

for file in files:
    with open(file, encoding='utf8') as f:
        rows = list(csv.reader(f))

    with open(file, 'w', encoding='utf8', newline='\r\n') as f:
        f.writelines(['%s,%s\n'%(word, cnt) for word, cnt in sorted([(x[0],x[1].replace(',','')) for x in rows], key=lambda x: (-int(x[1]), x[0]))])
