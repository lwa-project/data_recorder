#!/usr/bin/env python3

import os
import sys


drs = sys.argv[1:]
if len(drs) == 0:
    # Standard build
    os.system("make clean && make -j all")
else:
    for dr in drs:
        try:
            dr = int(dr, 10)
            dr = f"DR{dr}"
        except ValueError:
            dr = dr.upper()
            
        os.system(f"rm -rf Makefile.{dr}")
        with open("Makefile", 'r') as im:
            with open(f"Makefile.{dr}", 'w') as om:
                for line in im:
                    if line.startswith('INSTALL_LOCATION'):
                        line = f"INSTALL_LOCATION?=/LWA/{dr}\n"
                    elif line.startswith('STORAGE_LOCATION'):
                        line = f"STORAGE_LOCATION?=/LWA_STORAGE/{dr}\n"
                    om.write(line)
                    
        flags  = f" -DDEFAULT_CONFIG_FILE='\\\"/LWA/{dr}/config/defaults_v2.cfg\\\"'"
        flags += f" -DDEFAULT_TUNING_FILE='\\\"/LWA/{dr}/config/netperformance.sysctl.conf\\\"'"
        flags += f" -DDEFAULT_SCRIPT_DIR='\\\"/LWA/{dr}/scripts\\\"'"
        flags += f" -DDEFAULT_STORAGE_DIR='\\\"/LWA_STORAGE/{dr}\\\"'"
        
        os.system("make clean")
        os.system(f"CPPFLAGS=\"{flags}\" make -j all -f Makefile.{dr}")
