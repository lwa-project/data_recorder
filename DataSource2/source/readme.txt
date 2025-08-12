/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2010  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.

  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
*/

Software: MCS-DR Operating Software utility: DataSource (DP datastream emulator)
Author:    C.N. Wolfe
Version:  __VERSION__
Date:     __DATE__

Introduction
============

  DataSource is a command-line tool to emulate a DP data source.
  Data Source operates in one of 2 modes. The original mode (w/o the new -DRX flag) operates as before,
  producing packets of a specified size at a specified rate for a specified length of time. The new 
  interface emulates DRX-formatted packets containing sine waves, zero-mean gaussian noise, and a chirp 
  function (sweeping or oscillating sines). The output in this mode may be complex or real. Details of 
  each interface follow.
  
  Non-DRX mode usage
  ==================
  This mode produces UDP packets that contain: 
    a 64-bit unsigned packet identifier,
    a 64-bit unsigned packet milliseconds-past-midnight timestamp,
    a 64-bit unsigned key value (provided at command line -Key or -k)
    and (DataSize-24) bytes which cycle through all 256 possible 
    8-bit numbers, starting at 0x18 and resetting to 0 after reaching 256.
  
  Command line options for Non-DRX mode are as follows: 
  
  DataSource 
      -DestinationIpAddress aaa.bbb.ccc.ddd
      -DestinationPort 12345
      -DataSize 1234        (in bytes per packet)
      -Rate 123456789012345 (in bytes/second)
      -Duration 1234567890 (in milliseconds)
  
  
  DRX mode usage
  ==============
  This mode produces valid DRX formatted packets, at a specified data rate, each containing 4096 complex samples.
  No decimation or filtering is performed, and the "sample frequency" -- fs -- is independent of the supplied data 
  rate. Specifically, decFactor, freqCode, etc. are cosmetic, and are only used to fill the packets. All polarizations 
  and tunings will produce identical data frame content (but with appropriate headers). The exception to this is when
  the -XCP flag is supplied, wherein the Y polarization will be orthogonal to X.
   
  DataSource
    (Mandatory parameters in DRX mode)
        -DRX                      Mode Flag: enable DRX mode                         
        -i  aaa.bbb.ccc.ddd       Destination IP address           
        -p  12345                 Destination port                 
        -r  123456789012345       Rate (bytes/s                    
        -d  1234567890            Duration (in milliseconds)       
    (Optional parameters  in DRX mode)
        -df 1234                  decFactor                         
        -fc0 123456789.123        freq Code 0 
        -fc1 123456789.123        freq Code 1 
        -fs  123.45               Sample frequency for sines,   Hz (default 32)
        -f0  123.45               Sine wave 0 frequency,        Hz (default -5)
        -m0  123.45               Sine wave 0 amplitude,           (default 3.0)
        -f1  123.45               Sine wave 1 frequency,        Hz (default 5)
        -m1  123.45               Sine wave 1 amplitude,           (default 4.0)
        -f2  123.45               Sine wave 2 frequency,        Hz (default 14)
        -m2  123.45               Sine wave 2 amplitude,           (default 5.0)
        -cf0 123.45               Chirp frequency 0,            Hz (default -3)
        -cf1 123.45               Chirp frequency 1,            Hz (default 3)
        -cf2 123.45               Chirp sweep frequency,        Hz (default 0.000015)
        -cm  123.45               Chirp magnitude,                 (default 4)
        -cg  123.45               Chirp gamma,                     (default 1.0)
        -SAW 123.45               sweep instead of osc.            (default not set)
        -sig 123.45               Gaussian Noise Scale,            (default 1.0)
        -sc  123.45               scale range for all signals      (default 8.0)
        -N   123456               # frames to pre-generate         (default 19140)
        -useComplex X             flag: 1=complex 0=real-only      (default 0)
    (Ignored parameters  in DRX mode)
        -k  1234567890            Key                              
        -s  1234                  Packet size                      
  
  

  
Build instructions
==================
  This utility is automatically built and installed by in the process of building and installing the tarball. 
  To build just this package, the DataSource executable may be built by untarring, entering the Release 
  folder, and issuing make:
    >tar -xvf ./<Tarballl Filename>.tar.gz 
    >cd build/DataSource/Release/
    >touch ../*
    >make clean all
  Afterwards, it should be copied to /LWA/bin/DataSource with root ownership and 755 permissions.
  DataSource executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system. 
 