def parse_txt(fn):
    reqs = {}
    with open(fn, "r") as f:
        c = 0
        init = False
        url = ''
        for l in f.readlines():
            x = {}
            try:
                x = json.loads(l)
            except:
                print 'Error parsing line: ', l
            if 'params' in x:
                y = x['params']
                if init:
                    if c < 100:
                        if 'request' in y:
                            # print y['request']['url']
                            reqs[url].append(y['request']['url'])
                        elif 'type' in y:
                            if y['type'] == 'Script':
                                # print y['response']['url']
                                reqs[url].append(y['response']['url'])
                        elif 'url' in y:
                            # print y['url']
                            reqs[url].append(y['url'])
                        c = c + 1
                else:
                    if 'url' in y:
                        t = y['url'].strip('http://').strip('/')
                        u = y['url'].strip('https://').strip('/')
                        if t in url_list:
                            init = True
                            url = t
                        elif u in url_list:
                            init = True
                            url = u
                        reqs[url] = []
        return reqs


def parse():
    names = []
    for filename in os.listdir('raw'):
        if not filename.endswith('.txt'): continue
        fullname = os.path.join('raw', filename)
        names.append(fullname)
    results = []
    for name in names:
        results.append(parse_txt(name))
        # break
    for res in results:
        for u in res:
            if u not in url_list: continue
            print u, len(res[u])



