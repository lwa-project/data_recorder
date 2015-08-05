/*
    License Disclaimer
    This project includes "gnuplot_i.hpp", which is licensed under GPL_V2
    from N. Devillard, Rajarshi Guha, V. Chyzhdzenka, and M. Burgis 2003-2008. 
    All other sources/files are licensed under GPL_V3, by C. Wolfe and 
    Virginia Tech, 2012.
*/

/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2012  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

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

Software: MCS-DR Operating Software utility: Correlation Viewer Utility
Author:    C.N. Wolfe
Version:  DROS-1.5.0-Beta-Release
Date:     2012-06-22

Introduction
============
  CorrelationViewer is a userspace tool to display correlation data recorded with DROS. This is a pre-alpha 
  release of the utility, and may not perform exactly as expected. The known limitations are described in the 
  'Limitations' section. 
  
Assumptions
===========

  To use this utility, you must have installed the 'build-essential' and 'gnuplot' packages, on an x86_64 ubuntu 
  distribution. You must have a file containing the output of the DROS's 'XCP' command.   

Build instructions
==================
  This utility is automatically built and installed by in the process of building and installing the tarball. 
  To build just this package, the CorrelationViewer executable may be built by untarring, entering the Release 
  folder, and issuing make:
    >tar -xvf ./<Tarballl Filename>.tar.gz 
    >cd build/CorrelationViewer/Debug/
    >make clean all
    
  The CorrelationViewer executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system. 

	
Usage
================
    CorrelationViewer <file> <start> <count>
        <file>     the file name of the DROS-produced correlation recording
        <start>    index of the first correlation block
        <count>    number of correlation block
                   
    Start and count, combined determine the interval over which results are displayed, noting limitations below.
    
Known Limitations
=================
    * axes and scaling may not be computed correctly
    * various other display quirks 

File Inventory
==============
ReadMe.txt            This file.

.project              Eclipse-specific project settings file
.cproject             Eclipse-specific project settings file
/Debug/makefile       makefile
/Debug/objects.mk     file included by makefile during make
/Debug/sources.mk     file included by makefile during make
/Debug/subdir.mk      file included by makefile during make

CorrelationViewer     The main file, reads spectra, outputs plots
gnuplot_i.hpp          The gnuplot c++ interface class, slightly 
                       modified for displaying multiple time-series on the same plot.       
