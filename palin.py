prefixes = ['', '_with_some_prefixes']#, '_with_all_prefixes', '_with_all_prefixes_with_he_hasheela'] # note the last two files are 1.5GB and 2.2GB strong so they are not included in the repo

for prefix in prefixes:
    palin = []
    anad = []
    palin_anad = []
    file = 'all_with_fatverb%s.txt'%prefix
    with open(file, encoding='utf8') as f:
        lines = f.readlines()
        clean_lines = [line.strip().replace('"','').replace("'",'').replace('-','').replace('ך','כ').replace('ם','מ').replace('ן','נ').replace('ף','פ').replace('ץ','צ') for line in lines]
        clean_lines_set = set(clean_lines)
        for line, clean in zip(lines, clean_lines):
            if clean==clean[::-1]:
                palin.append(line)
                palin_anad.append(line)
            elif clean[::-1] in clean_lines_set:
                anad.append(line)
                palin_anad.append(line)
    palin = sorted(list(dict.fromkeys(palin)))
    anad = sorted(list(dict.fromkeys(anad)))
    palin_anad = sorted(list(dict.fromkeys(palin_anad)))
    palin[-1] = palin[-1].strip()
    anad[-1] = anad[-1].strip()
    palin_anad[-1] = palin_anad[-1].strip()
    maxp = max(len(x.strip().replace('"','').replace("'",'').replace('-','')) for x in palin)
    maxa = max(len(x.strip().replace('"','').replace("'",'').replace('-','')) for x in anad)
    print('wordlist: %s'%file)
    print('longest palindromes with %d chars:'%maxp, [x.strip() for x in palin if len(x.strip().replace('"','').replace("'",'').replace('-',''))==maxp])
    print('longest proper anadromes with %d chars:'%maxa, [x.strip() for x in anad if len(x.strip().replace('"','').replace("'",'').replace('-',''))==maxa])
    print('palin=%d anad=%d both=%d\n'%(len(palin),len(anad),len(palin)+len(anad)))
    with open('palindromes%s.txt'%prefix,'w',encoding='utf8') as f:
        f.writelines(palin)
    with open('anadromes%s.txt'%prefix,'w',encoding='utf8') as f:
        f.writelines(anad)
    with open('palin_and_anad%s.txt'%prefix,'w',encoding='utf8') as f:
        f.writelines(palin_anad)
