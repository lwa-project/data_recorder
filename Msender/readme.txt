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
Software: MCS-DR Operating Software utility: Msender
Author:    C.N. Wolfe
Version:  DROS-2.0-Devel-31614
Date:     

Introduction / Usage
====================

  Msender is a command-line tool to communicate with MCS Common-ICD compliant subsystems.
  It sends a message to the specified system with the specified contents and waits for a response,
  and prints the data field of the received message. 
  command line options are:
      -Source XXX                                Reference designator of the system sending the message
      -Destination YYY                           Reference designator of the system receiving the message
      -Type ZZZ                                  Command Type
      -ReferenceNumber 123456789                 Reference Number
      -DestinationIpAddress aaa.bbb.ccc.ddd      URL or ip address to send message to
      -DestinationPort 12345                     Messaging port on the remote system
      -ResponseListenPort 12345                  Messaging port to listen on
      -Data "string with no embedded nulls"      (Optional) data contents of sent message's
      -v                                         (Optional) verbose mode, display sent message and received message
      
      -expectedMPM 123456789                     (Optional) Test response MPM for match  
      -expectedMJD 123456                        (Optional) Test response MJD for match
      -expectedSender ABC                        (Optional) Test response sender field for match
      -expectedDestination ABC                   (Optional) Test response destination field for match
      -expectedType ABC                          (Optional) Test response command type for match
      -expectedStatus ABCDEFG                    (Optional) Test response status field for match
      -expectedReference 123456789               (Optional) Test response reference number for match
      -expectedAccepted "Accepted" or "Rejected" (Optional) Test response for "A" or "R"
      -expectedComment SOME_ASCII_TEXT           (Optional) Test response MPM for exact match 
  
  If any of the expectedXXX options are specified, the output will contain "[PASS]" or "[FAIL]" according 
  to whether ALL of the specified fields matched. 
  
  Additionally, data field text containing "MPM + XXX", or "MJD + XXX" will add XXX to the current MPM or MJD 
  and substitute at that location before sending the message.
  
  If no response is received, this application will hang and have to be terminated (CTRL+C or 'killall Msender')
  
  
Build instructions
==================
  This utility is automatically built and installed by in the process of building and installing the tarball. 
  To build just this package, the Msender executable may be built by untarring, entering the Release 
  folder, and issuing make:
    >tar -xvf ./<Tarballl Filename>.tar.gz 
    >cd build/Msender/Release/
    >touch ../*
    >make clean all
  Afterwards, it should be copied to /LWA/bin/Msender with root ownership and 755 permissions.
  Msender executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system. 
 