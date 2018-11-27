import os, sys, getopt, json
from ipwhois import IPWhois
import socket,struct
import pprint
from urlparse import urlparse


def makeMask(n):
    "return a mask of n bits as a long integer"
    return (2L<<n-1) - 1

def dottedQuadToNum(ip):
    "convert decimal dotted quad string to long integer"
    # print ip
    # print socket.inet_aton(ip)
    return struct.unpack('<L',socket.inet_aton(ip))[0]

def networkMask(ip,bits):
    "Convert a network address to a long integer" 
    return dottedQuadToNum(ip) & makeMask(bits)

def addressInNetwork(ip,net):
   "Is an address in a network"
   # print ip, net
   return ip & net == net

url_list = ['www.nytimes.com', 
        'www.bbc.com', 
        'www.ft.com', 
        'www.msn.com', 
        'www.github.com', 
        'www.reddit.com', 
        'www.yahoo.com', 
        'www.washingtonpost.com', 
        'www.latimes.com', 
        'www.cnn.com', 
        'www.theguardian.com', 
        'www.independent.co.uk', 
        'npr.org', 
        'www.cnbc.com', 
        'www.time.com', 
        'www.cbsnews.com', 
        'cbc.ca', 
        'www.abcnews.go.com', 
        'www.vox.com',
        'www.rawstory.com']



def checkList(l, addr):
    for i in l:
        mask = networkMask(i, 24)
        if addressInNetwork(addr, mask):
            return True
    return False

def parse_ips(fn):
    ips = {}
    with open(fn, "r") as f:
        for l in f.readlines():
            s = l.split('Dst:')
            if len(s) > 1:
                ip = s[1].strip()
                address = dottedQuadToNum(ip)
                if not checkList(ips.keys(), address):
                    try:
                        ips[ip] = IPWhois(ip).lookup_rdap(depth=1)['objects'].items()
                    except:
                        print 'error during lookup'
    return ips

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
                    # if c < 1000:
                    try:
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
                    except:
                        print 'error'
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
    # for res in results:
    #     for u in res:
    #         if u not in url_list: continue
    #         print u, len(res[u])
    return results


def main(argv):
    msg = 'parse.py'
    try:
        opts, args = getopt.getopt(argv, "h", [])
    except getopt.GetoptError:
        print msg
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print msg
            sys.exit()

    ###start of stuff
    print 'Parsing test results...'
    reqs = parse()
    # print 'Parsing ips...'
    # ips = parse_ips('ips.txt')
    # with open('ip_info.txt', 'w') as f:
    #     for ip in ips:
    #         f.write(ip + ',')
    #         f.write(json.dumps(ips[ip]))
    #         f.write('\n')
    info = {}
    with open('ip_info.txt', 'r') as f:
        for l in f.readlines():
            s = l.split(',', 1)
            info[s[0]] = json.loads(s[1]) 
    names = {}
    for ip in info:
        if 'contact' in info[ip][0][1]:
            # print info[ip][0][1]['contact']['name']
            names[info[ip][0][1]['contact']['name']] = ip
     
    answers = {}
    for r in reqs:
        for request in r:
            if request == '': continue
            # print request
            answers[request] = []
            for sub in r[request]:
                s = sub.upper()
                for x in names:
                    if x == 'Abuse': continue
                    if x == 'NOC': continue
                    if x.upper() in s:
                        parsed = urlparse(sub)
                        d = '{uri.scheme}://{uri.netloc}/'.format(uri=parsed)
                        answers[request].append((x, names[x], d))
                if 'EDGECAST' in s:
                    answers[request].append('Verizon EdgeCast')
    # with open('results.txt', 'w') as f:
    #     f.write(json.dumps(answers))
    pprint.pprint(answers)


if __name__ == "__main__":
    main(sys.argv[1:])
