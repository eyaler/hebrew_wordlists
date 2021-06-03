palin = []
anad = []
palin_anad = []
with open('all_with_fatverb.txt', encoding='utf8') as f:
    lines = f.readlines()
    clean_lines = [line.strip().replace('"','').replace("'",'') for line in lines]
    clean_lines_set = set(clean_lines)
    for line, clean in zip(lines, clean_lines):
        if clean==clean[::-1]:
            palin.append(line)
            palin_anad.append(line)
        elif clean[::-1] in clean_lines_set:
            anad.append(line)
            palin_anad.append(line)
print('palin=%d anad=%d'%(len(palin),len(anad)))
with open('palindromes.txt','w',encoding='utf8') as f:
    f.writelines(palin)
with open('anadromes.txt','w',encoding='utf8') as f:
    f.writelines(anad)
with open('palin_and_anad.txt','w',encoding='utf8') as f:
    f.writelines(palin_anad)
