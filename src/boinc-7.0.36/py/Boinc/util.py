## $Id: util.py 2245 2003-09-04 03:19:59Z quarl $

def list2dict(list):
    dict = {}
    for k in list: dict[k] = None
    return dict

def sorted_keys(dict):
    k = dict.keys()
    k.sort()
    return k
