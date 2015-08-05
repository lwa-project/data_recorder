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

Software: MCS-DR Operating Software utility: Spectrogram Viewer Utility
Author:    C.N. Wolfe
Version:  DROS-2.0-Devel-31614
Date:     

Introduction
============
  SpectrogramViewer is a userspace tool to display spectrogram data recorded with DROS. This is a pre-alpha 
  release of the utility, and may not perform exactly as expected. The known limitations are described in the 
  'Limitations' section. 
  

Assumptions
===========

  To use this utility, you must have installed the 'build-essential' and 'gnuplot' packages, on an x86_64 ubuntu 
  distribution. You must have a file containing the output of the DROS's 'SPC' command.   

Build instructions
==================
  This utility is automatically built and installed by in the process of building and installing the tarball. 
  To build just this package, the SpectrogramViewer executable may be built by untarring, entering the Release 
  folder, and issuing make:
    >tar -xvf ./<Tarballl Filename>.tar.gz 
    >cd build/SpectrogramViewer/Release/
    >make clean all
    
  The SpectrogramViewer executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system. 

	
Usage
================
    SpectrogramViewer <file> <start> <count>
        <file>     the file name of the DROS-produced spectra recording
        <start>    index of the first spectra
        <count>    number of spectra
                   
    Start and count, combined determine the interval over which spectra are 
    integrated for the integrated-spectra plots. They also determine the 
    time domain of time-series plots, noting limitations below.
    
Known Limitations
=================
    * timescale is currently hard-coded to ~10ms/spectra
    * breaks when using other than 32 channels
    * does not handle gaps in files (i.e. missing spectra)
    * may flake out with scaling of plots
    * axes and scaling may not be computed correctly
    * axes of individual pols/tunings not shown on identical ranges
    * various other display quirks 

File Inventory
==============
ReadMe.txt              This file.

.project                Eclipse-specific project settings file
.cproject               Eclipse-specific project settings file
/Release/makefile       makefile
/Release/objects.mk     file included by makefile during make
/Release/sources.mk     file included by makefile during make
/Release/subdir.mk      file included by makefile during make

SpectrogramViewer.cpp   The main file, reads spectra, outputs plots
gnuplot_i.hpp           The gnuplot c++ interface class, slightly 
                        modified for displaying waterfalls.       
