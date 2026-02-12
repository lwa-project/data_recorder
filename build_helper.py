#!/usr/bin/env python3

import os
import sys
import argparse


parser = argparse.ArgumentParser(
            description='script to help build by DROS for one or DRs', 
            formatter_class=argparse.ArgumentDefaultsHelpFormatter
            )
parser.add_argument('drs', type=int, nargs='*',
                    help='Numeric DR Ids to build DROS for - empty for only a single DR')
parser.add_argument('-n', '--n-stand', type=int, default=256,
                    help='Number of stands in the station for the TBT/TBS modes')
args = parser.parse_args()

with open('DROS2/Data/LwaDataFormats.h', 'r') as ih:
    with open('LwaDataFormats.h.tmp', 'w') as oh:
        for line in ih:
            if line.find('#define TBX_STAND_COUNT') != -1:
                line = f"#define TBX_STAND_COUNT {args.n_stand}\n"
            oh.write(line)
os.rename('DROS2/Data/LwaDataFormats.h', 'DROS2/Data/LwaDataFormats.h.orig')
os.rename('LwaDataFormats.h.tmp', 'DROS2/Data/LwaDataFormats.h')

with open('DataSource2/source/Lwa/LWA.h', 'r') as ih:
    with open('LWA.h.tmp', 'w') as oh:
        for line in ih:
            if line.find('#define TBX_STAND_COUNT') != -1:
                line = f"#define TBX_STAND_COUNT {args.n_stand}\n"
            oh.write(line)
os.rename('DataSource2/source/Lwa/LWA.h', 'DataSource2/source/Lwa/LWA.h.orig')
os.rename('LWA.h.tmp', 'DataSource2/source/Lwa/LWA.h')

if len(args.drs) == 0:
    # Standard build
    os.system("make clean && make -j all")
else:
    # Multi-DR build
    ## Save where we are at
    orig_dir = os.getcwd()
    
    ## Iterate over DRs to build
    for dr in args.drs:
        ### Name
        dr = f"DR{dr}"
        
        ### Build directory that we copy things into
        if not os.path.exists(f"build_{dr}"):
            os.mkdir(f"build_{dr}")
        os.system(f"rsync -avH --exclude build_* --exclude .git* . build_{dr}")
        
        ### Move into that build directory
        os.chdir(f"build_{dr}")
        
        ### Makefile update
        os.system(f"rm -rf Makefile.{dr}")
        with open("Makefile", 'r') as im:
            with open(f"Makefile.{dr}", 'w') as om:
                for line in im:
                    if line.startswith('INSTALL_LOCATION'):
                        line = f"INSTALL_LOCATION?=/LWA/{dr}\n"
                    elif line.startswith('STORAGE_LOCATION'):
                        line = f"STORAGE_LOCATION?=/LWA_STORAGE/{dr}\n"
                    line = line.replace("StartDROS.sh", f"StartDROS_{dr}.sh")
                    om.write(line)
        os.unlink("Makefile")
        os.rename(f"Makefile.{dr}", "Makefile")
        
        ### StartDROS.sh update
        with open("dros.service", 'r') as im:
            with open(f"dros-{dr.lower()}.service", 'w') as om:
                for line in im:
                    if line.find('/LWA/') != -1:
                        line = line.replace("/LWA/", f"/LWA/{dr}/")
                    elif line.find("=dros") != -1:
                        line = line.replace("=dros", f"=dros-{dr.lower()}")
                    elif line.find('=DROS') != -1:
                        line = line.replace("=DROS", f"=DROS - {dr}")
                    om.write(line)
        os.unlink("dros.service")
        
        ### Build flags
        flags  = f" -DDEFAULT_CONFIG_FILE='\\\"/LWA/{dr}/config/defaults_v2.cfg\\\"'"
        flags += f" -DDEFAULT_LOG_FILE='\\\"/LWA/{dr}/runtime/runtime.log\\\"'"
        flags += f" -DDEFAULT_TUNING_FILE='\\\"/LWA/{dr}/config/netperformance.sysctl.conf\\\"'"
        flags += f" -DDEFAULT_SCRIPT_DIR='\\\"/LWA/{dr}/scripts\\\"'"
        flags += f" -DDEFAULT_STORAGE_DIR='\\\"/LWA_STORAGE/{dr}\\\"'"
        
        ### Clean and build
        os.system("make clean")
        os.system(f"CPPFLAGS=\"{flags}\" make -j all")
        
        os.chdir(orig_dir)
