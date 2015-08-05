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

Software: MCS-DR Operating Software utility: Emulated Data File Verifier
Author:    C.N. Wolfe
Version:  DROS-1.5.0-Beta-Release
Date:     2012-06-22

Introduction
============
  This tool is used to verify files which were made by recording data from 
  the DataSource (DP emulator) tool and then copying that data to an 
  accessible native file system. (i.e. REC XXX -> CPY XXX)
  

Assumptions
===========
  That data produced with the DataSource utility has been recorded with the DR
  and subsequently CPY'ed or DMP'ed to some ext2/3 partition and is accessible 
  to this utility.  
  
   ******************************** Note ******************************
  * This utility will not work with data generated using the -DRX flag *
   ******************************** Note ******************************

Build instructions
==================
  This utility is automatically built and installed by in the process of building and installing the tarball. 
  To build just this package, the LWAFS reader executable may be built by untarring, entering the Release 
  folder, and issuing make:
    >tar -xvf ./<Tarball Filename>.tar.gz 
    >cd DataSourceFileVerifier/Release/
    >touch ../*
    >make
  The DataSourceFileVerifier executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system. 

    
Usage and Limitations
=====================
    
    To run the program, at the prompt, type:
    >DataSourceFileVerifier <packet size> <filename>
    
    Verification will begin. The process verifies that each subsequent chunk of <packet size> bytes contains:
        A) a 64-bit unsigned originating timestamp
        B) a 64-bit unsigned serial identifier (increasing by 1 per packet)
        C) a 64-bit unsigned sequence identifier (corresponds to unique ID supplied to DataSource)
        D) the remaining bytes of the packet filled with an 8-bit counter whose value
           should match the byte-address modulo 256.
    
    Errors detected in (D) will terminate the verification early, all others will be noted
    but will allow the remainder of the packet to be verified. (A) my change sonewhat non-deterministically,
    (B) should always increase by 1, and (C) should not change throughout the recording.
    
    

