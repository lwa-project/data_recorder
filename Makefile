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
	sudo apt-get install build-essential libgdbm-dev libgdbm3 libfuse-dev libfuse2 lm-sensors smartmontools mdadm

all:
	make -C $(WORKSPACE)/Msender/Release
	make -C $(WORKSPACE)/DataSource/Release
	make -C $(WORKSPACE)/DataSourceFileVerifier/Release
#	make -C $(WORKSPACE)/WAVE_Z_SYSTEM/Release
#	make -C $(WORKSPACE)/WAVE_Z_RECORD/Release
#	make -C $(WORKSPACE)/lwafs-all-in-one/Release
	make -C $(WORKSPACE)/WAVE_Y/Release
	make -C $(WORKSPACE)/FUSE-LWAFS-READER/Release
	make -C $(WORKSPACE)/SpectrogramViewer/Release
	make -C $(WORKSPACE)/CorrelationViewer/Debug

install:
	cp $(INSTALL_LOCATION)/config/defaults.cfg ./defaults.cfg	
	cp $(INSTALL_LOCATION)/config/formats.cfg ./formats.cfg
	install -b -g root -o root -d /LWADATA	
	rm -fr $(INSTALL_LOCATION) 
	install -b -g root -o root -d $(INSTALL_LOCATION)/bin
	install -b -g root -o root -d $(INSTALL_LOCATION)/config
	install -b -g root -o root -d $(INSTALL_LOCATION)/scripts
	install -b -g root -o root -d $(INSTALL_LOCATION)/database
	install -b -g root -o root -d $(INSTALL_LOCATION)/runtime
#	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/bin $(WORKSPACE)/Msender/Release/Msender $(WORKSPACE)/DataSource/Release/DataSource $(WORKSPACE)/DataSourceFileVerifier/Release/DataSourceFileVerifier $(WORKSPACE)/WAVE_Y/Release/WAVE_Y $(WORKSPACE)/FUSE-LWAFS-READER/Release/FUSE-LWAFS-READER $(WORKSPACE)/SpectrogramViewer/Release/SpectrogramViewer $(FILES)/get_barcode.py
	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/bin $(WORKSPACE)/Msender/Release/Msender $(WORKSPACE)/DataSource/Release/DataSource $(WORKSPACE)/DataSourceFileVerifier/Release/DataSourceFileVerifier $(WORKSPACE)/WAVE_Y/Release/WAVE_Y $(WORKSPACE)/FUSE-LWAFS-READER/Release/FUSE-LWAFS-READER $(WORKSPACE)/SpectrogramViewer/Release/SpectrogramViewer $(WORKSPACE)/CorrelationViewer/Debug/CorrelationViewer
#	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/bin $(WORKSPACE)/Msender/Release/Msender $(WORKSPACE)/DataSource/Release/DataSource $(WORKSPACE)/WAVE_Z_SYSTEM/Release/WAVE_Z_SYSTEM $(WORKSPACE)/WAVE_Z_RECORD/Release/WAVE_Z_RECORD $(WORKSPACE)/lwafs-all-in-one/Release/lwafs-all-in-one
	install -b -g root -o root -m 644 -t $(INSTALL_LOCATION)/config $(FILES)/defaults.cfg.example $(FILES)/defaults.cfg $(FILES)/formats.cfg $(FILES)/formats.cfg.example $(FILES)/netperformance.sysctl.conf
	install -b -g root -o root -m 544 -t $(INSTALL_LOCATION)/scripts $(FILES)/ICD-Compliance-Test.sh $(FILES)/MakeLoopback.sh $(FILES)/RemoveLoopback.sh $(FILES)/installStartupScript.sh $(FILES)/uninstallStartupScript.sh $(FILES)/StartDROS.sh $(FILES)/ScanForMultiDiskArrays.sh
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
	@echo "#       $(INSTALL_LOCATION)/bin/WAVE_Y"
	@echo "# "
	@echo "# To install the software to run on-boot, execute:"
	@echo "#       $(INSTALL_LOCATION)/scripts/installStartupScript.sh $(INSTALL_LOCATION)/scripts/StartDROS.sh"
	@echo "# "
	@echo "################################################################"

clean:
	make -C $(WORKSPACE)/Msender/Release clean
	make -C $(WORKSPACE)/DataSource/Release clean
	make -C $(WORKSPACE)/DataSourceFileVerifier/Release clean
#	make -C $(WORKSPACE)/WAVE_Z_SYSTEM/Release clean
#	make -C $(WORKSPACE)/WAVE_Z_RECORD/Release clean
#	make -C $(WORKSPACE)/lwafs-all-in-one/Release clean
	make -C $(WORKSPACE)/WAVE_Y/Release clean
	make -C $(WORKSPACE)/FUSE-LWAFS-READER/Release clean
	make -C $(WORKSPACE)/SpectrogramViewer/Release clean
	make -C $(WORKSPACE)/CorrelationViewer/Debug clean


	

