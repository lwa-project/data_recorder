#!/usr/bin/env python3

import os
import sys
import time
from socket import gethostname

from lwa_auth import KEYS as LWA_AUTH_KEYS
from lwa_auth.signed_requests import post as signed_post

URL = "https://lda10g.unm.edu/metadata/sorter/upload"
SITE = gethostname().split('-', 1)[0]
SUBSYSTEM = gethostname().split('-', 1)[1].upper()
TYPE = "SSLOG"

# Send the update to lwalab
r = os.path.realpath(sys.argv[1])
if r.find('/LWA/DR') != -1:
    SUBSYSTEM = r.split('/')[2].upper()
f = signed_post(LWA_AUTH_KEYS.get('dr', kind='private'), URL,
                data={'site': SITE, 'type': TYPE, 'subsystem': SUBSYSTEM},
                files={'file': open(r)},
                verify=False) # We don't have a certiticate for lda10g.unm.edu
print(f.text)
f.close()
