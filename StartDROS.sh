#!/bin/bash
touch /LWA/runtime/start.info
echo "`date`: Entered startup with parameter '$1'" >> /LWA/runtime/start.info
case "$1" in
'start')
  #install network performance system variables
  sysctl -e -p /LWA/config/netperformance.sysctl.conf
  #rm -f /LWA/runtime/barcodes
  #/LWA/bin/get_barcode.py | grep -v Warning | grep -v "device error" > /LWA/runtime/barcodes

  #install temp sensor modules
  modprobe -q coretemp
  modprobe -q f71882fg
  mkdir -p /LWA/runtime
  rm -f /LWA/runtime/*.log
  rm -f /LWA/runtime/okgo
  /LWA/scripts/ScanForMultiDiskArrays.sh > /LWA/runtime/BootupArrayScan.log
  if [ -e /LWA/runtime/okgo ]; then
    echo "Starting DROS ..." > /LWA/runtime/runtime.log
    /LWA/bin/WAVE_Y -primaryLaunch >> /LWA/runtime/runtime.log
  else
    echo "Could not start DROS due to missing DRSU" > /LWA/runtime/runtime.log
  fi

;;
esac
