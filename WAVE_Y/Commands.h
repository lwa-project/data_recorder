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
/*
 * Commands.h
 *
 *  Created on: Oct 30, 2009
 *      Author: chwolfe2
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_
#include "Defines.h"
#include "Message.h"

StatusCode Command_Process(SafeMessage* message, SafeMessage* response);

StatusCode Command_PNG(SafeMessage* message, SafeMessage* response);
StatusCode Command_RPT(SafeMessage* message, SafeMessage* response);
StatusCode Command_SHT(SafeMessage* message, SafeMessage* response);

StatusCode Command_INI(SafeMessage* message, SafeMessage* response);
StatusCode Command_REC(SafeMessage* message, SafeMessage* response);
StatusCode Command_FakeREC(SafeMessage* message, SafeMessage* response);
StatusCode Command_DEL(SafeMessage* message, SafeMessage* response);
StatusCode Command_STP(SafeMessage* message, SafeMessage* response);
StatusCode Command_GET(SafeMessage* message, SafeMessage* response);
StatusCode Command_CPY(SafeMessage* message, SafeMessage* response);
StatusCode Command_DMP(SafeMessage* message, SafeMessage* response);
StatusCode Command_FMT(SafeMessage* message, SafeMessage* response);
StatusCode Command_DWN(SafeMessage* message, SafeMessage* response);
StatusCode Command_UP (SafeMessage* message, SafeMessage* response);
StatusCode Command_EJT(SafeMessage* message, SafeMessage* response);
StatusCode Command_SYN(SafeMessage* message, SafeMessage* response);
StatusCode Command_TST(SafeMessage* message, SafeMessage* response);
StatusCode Command_VFY(SafeMessage* message, SafeMessage* response);
StatusCode Command_SEL(SafeMessage* message, SafeMessage* response);
StatusCode Command_EXT(SafeMessage* message, SafeMessage* response);
StatusCode Command_BUF(SafeMessage* message, SafeMessage* response);
StatusCode Command_ECH(SafeMessage* message, SafeMessage* response);
StatusCode Command_JUP(SafeMessage* message, SafeMessage* response);
StatusCode Command_XCP(SafeMessage* message, SafeMessage* response);
StatusCode Command_SPC(SafeMessage* message, SafeMessage* response);
StatusCode Command_DVN(SafeMessage* message, SafeMessage* response);
#endif /* COMMANDS_H_ */
