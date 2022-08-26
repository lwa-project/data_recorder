#!/usr/bin/env python3

import os
import sys


drs = sys.argv[1:]
if len(drs) == 0:
    # Standard build
    os.system("make clean && make -j all")
else:
    # Multi-DR build
    ## Save where we are at
    orig_dir = os.getcwd()
    
    ## Iterate over DRs to build
    for dr in drs:
        ### Name
        try:
            dr = int(dr, 10)
            dr = f"DR{dr}"
        except ValueError:
            dr = dr.upper()
            
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
        with open("StartDROS.sh", 'r') as im:
            with open(f"StartDROS_{dr}.sh", 'w') as om:
                for line in im:
                    if line.startswith('### END INIT INFO'):
                        line += "\n"
                        line += "\n"
                        line += "# determine the storage path from Config.h\n"
                        line += f"CONFIG_PATH=/LWA/{dr}/Config.sh\n"
                        line += "source ${CONFIG_PATH}\n"
                        line += "\n"
                    elif line.find('/LWA/') != -1:
                        line = line.replace("/LWA/", f"/LWA/{dr}/")
                    elif line.find('/LWA_UPGRADES/') != -1:
                        line = line.replace("/LWA_UPGRADES/", f"/LWA_UPGRADES/{dr}/")
                    elif line.find("StartDROS") != -1:
                        line = line.replace("StartDROS", f"StartDROS_{dr}")
                    elif line.find(' DROS') != -1:
                        line = line.replace(" DROS", f" DROS - {dr}")
                    om.write(line)
        st = os.stat("StartDROS.sh")
        os.chown(f"StartDROS_{dr}.sh", st.st_uid, st.st_gid)
        os.chmod(f"StartDROS_{dr}.sh", st.st_mode)
        os.unlink("StartDROS.sh")
        
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
