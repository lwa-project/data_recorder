####################################################################
#
# MCS-DR DROS & Utilities makefile
# Author:  Christopher Wolfe
# Company: Virginia Tech
# Date:    12 FEB 10
# Purpose: To build/install LWA MCS-DROS software from tarball
#          This makefile invokes the eclipse-generated makefiles of 
#          the various projects associated with the DROS, included in this tarball
#          Primary DROS binary is called WAVE_Y
#
####################################################################

WORKSPACE=.
#WORKSPACE=/home/chwolfe2/workspace-3.4
INSTALL_LOCATION=/LWA
FILES=.

deps:
	sudo apt-get install build-essential libgdbm-dev libgdbm3 libfuse-dev libfuse2 lm-sensors smartmontools mdadm libboost-all-dev libfftw3

all:
	make -C $(WORKSPACE)/Msender/Release
	make -C $(WORKSPACE)/DataSource/Release
	make -C $(WORKSPACE)/DataSource_v2/Debug
	make -C $(WORKSPACE)/DROS2/Debug
	make -C $(WORKSPACE)/SpectrogramViewer/Release
	make -C $(WORKSPACE)/SpectrogramViewer2/Release

backup_config:
	@if [ -f "$(INSTALL_LOCATION)/config/defaults_v2.cfg" ]; then echo "Backing up current configuration..."; cp $(INSTALL_LOCATION)/config/defaults_v2.cfg /BACKUP.defaults_v2.cfg; fi

install: backup_config
#	cp $(INSTALL_LOCATION)/config/formats.cfg ./formats.cfg
#	install -b -g root -o root -d /LWADATA	
	install -b -g root -o root -d /LWA_STORAGE
	rm -fr $(INSTALL_LOCATION) 
	install -b -g root -o root -d $(INSTALL_LOCATION)/bin
	install -b -g root -o root -d $(INSTALL_LOCATION)/config
	install -b -g root -o root -d $(INSTALL_LOCATION)/scripts
	install -b -g root -o root -d $(INSTALL_LOCATION)/runtime
	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/bin $(WORKSPACE)/Msender/Release/Msender $(WORKSPACE)/DataSource/Release/DataSource $(WORKSPACE)/DataSource_v2/Debug/DataSource_v2 $(WORKSPACE)/DROS2/Debug/DROS2 $(WORKSPACE)/SpectrogramViewer/Release/SpectrogramViewer $(WORKSPACE)/SpectrogramViewer2/Release/SpectrogramViewer2 
	install -b -g root -o root -m 644 -t $(INSTALL_LOCATION)/config $(FILES)/defaults_v2.cfg.example $(FILES)/defaults_v2.cfg $(FILES)/netperformance.sysctl.conf
	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/scripts $(FILES)/installStartupScript.sh $(FILES)/uninstallStartupScript.sh $(FILES)/StartDROS.sh $(WORKSPACE)/DROS2/Scripts/StorageControl.sh
	@if [ -f "/BACKUP.defaults_v2.cfg" ]; then echo "Restoring current configuration..."; cp /BACKUP.defaults_v2.cfg $(INSTALL_LOCATION)/config/defaults_v2.cfg; fi
	@echo 
	@echo "################################################################"
	@echo "# Notice:"
	@echo "# "
	@echo "# MCS-DR software has been installed to:"
	@echo "#       $(INSTALL_LOCATION)"
	@echo "# "
	@echo "# The default configuration files are installed to:"
	@echo "#       $(INSTALL_LOCATION)/config/"
	@echo "# "
	@echo "# You must modify defaults.cfg.example to reflect your network environment."
	@echo "# "
	@echo "# To launch the software, execute:"
	@echo "#       $(INSTALL_LOCATION)/bin/DROS2"
	@echo "# "
	@echo "# To install the software to run on-boot, execute:"
	@echo "#       cd $(INSTALL_LOCATION)/scripts; ./installStartupScript.sh StartDROS.sh"
	@echo "# "
	@echo "################################################################"

clean:
	make -C $(WORKSPACE)/Msender/Release clean
	make -C $(WORKSPACE)/DataSource/Release clean
	make -C $(WORKSPACE)/DataSource_v2/Debug clean
	make -C $(WORKSPACE)/DROS2/Debug clean
	make -C $(WORKSPACE)/SpectrogramViewer/Release clean
	make -C $(WORKSPACE)/SpectrogramViewer2/Release clean


