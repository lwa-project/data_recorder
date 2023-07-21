####################################################################
#
# MCS-DR DROS & Utilities makefile
# Author:  Christopher Wolfe
# Company: Virginia Tech
# Date:    12 FEB 10
# Purpose: To build/install LWA MCS-DROS software from tarball
##@TAB@#  This makefile invokes the eclipse-generated makefiles of 
##@TAB@#  the various projects associated with the DROS, included in this tarball
##@TAB@#  Primary DROS binary is called WAVE_Y
#
####################################################################

WORKSPACE=.
#WORKSPACE=/home/chwolfe2/workspace-2013
INSTALL_LOCATION?=/LWA
STORAGE_LOCATION?=/LWA_STORAGE
FILES=.

RM := rm -rf

all:
	make -j 6 -C $(WORKSPACE)/Msender/Release
	make -j 6 -C $(WORKSPACE)/DataSource/Release
	make -j 6 -C $(WORKSPACE)/DataSource_v2/Debug
	make -j 6 -C $(WORKSPACE)/DROS2/Debug all
	make -j 6 -C $(WORKSPACE)/SpectrogramViewer/Release

docs:
	$(WORKSPACE)/DROS2/doxygen.sh

deps:
	sudo apt-get install build-essential libgdbm-dev libgdbm3 libfuse-dev libfuse2 lm-sensors smartmontools mdadm libboost-all-dev libfftw3-dev

backup_config:
	@if [ -f "$(INSTALL_LOCATION)/config/defaults_v2.cfg" ]; then echo "Backing up current configuration..."; cp $(INSTALL_LOCATION)/config/defaults_v2.cfg /BACKUP.defaults_v2.cfg; fi

install: backup_config
#	cp $(INSTALL_LOCATION)/config/formats.cfg ./formats.cfg
#	install -b -g root -o root -d /LWADATA	
	install -b -g root -o root -d $(STORAGE_LOCATION)
	rm -fr $(INSTALL_LOCATION) 
	install -b -g root -o root -d $(INSTALL_LOCATION)/bin
	install -b -g root -o root -d $(INSTALL_LOCATION)/config
	install -b -g root -o root -d $(INSTALL_LOCATION)/scripts
	install -b -g root -o root -d $(INSTALL_LOCATION)/runtime
	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/bin \
		$(WORKSPACE)/Msender/Release/Msender \
		$(WORKSPACE)/DataSource/Release/DataSource \
		$(WORKSPACE)/DataSource_v2/Debug/DataSource_v2 \
		$(WORKSPACE)/DROS2/Debug/DROS2-LiveBuffer \
		$(WORKSPACE)/DROS2/Debug/DROS2-Spectrometer \
		$(WORKSPACE)/SpectrogramViewer/Release/SpectrogramViewer 
	install -b -g root -o root -m 644 -t $(INSTALL_LOCATION)/config \
		$(FILES)/defaults_v2.cfg.example \
		$(FILES)/defaults_v2.cfg \
		$(FILES)/netperformance.sysctl.conf
	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/scripts \
		$(FILES)/installStartupScript.sh \
		$(FILES)/uninstallStartupScript.sh \
		$(FILES)/StartDROS*.sh \
		$(FILES)/uploadLogfile.py \
		$(FILES)/uploadLogfile.sh \
		$(WORKSPACE)/DROS2-Spectrometer/Scripts/StorageControl.sh
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
	-$(RM) $(WORKSPACE)/DROS2/Docs/*
	-$(RM) $(WORKSPACE)/build_DR*
