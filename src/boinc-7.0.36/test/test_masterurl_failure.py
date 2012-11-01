#!/usr/bin/env python

## $Id: test_masterurl_failure.py 1655 2003-07-10 00:35:51Z quarl $

from test_uc import *

if __name__ == '__main__':
    test_msg("scheduler exponential backoff (master url failure)")
    Proxy('close_connection if $nconnections < 2; ', html=1)
    # Proxy(('print "hi nc=$nconnections start=$start nchars=$nchars url=$url\n"; ' +
    #        'close_connection if $nconnections < 2; '),
    #       html=1)
    # Proxy(( 'print "hi nc=$nconnections start=$start nchars=$nchars url=$url\n"; ' +
    #        ''),
    #       html=1)
    ProjectUC(short_name='test_masterurl_failure')
    run_check_all()

    ## TODO: verify it took ??? seconds
