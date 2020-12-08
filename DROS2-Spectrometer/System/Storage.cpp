// ========================= DROSv2 License preamble===========================
// Software for DROSv2 is covered jointly under GPLv3 and Boost 1.0 licenses,
// to the extent required by included Boost sources and GPL sources, and to the
// more restrictive case pertaining thereunto, as defined herebelow. Beyond those
// requirements, any code not explicitly restricted by either of thowse two license
// models shall be deemed to be licensed under the GPLv3 license and subject to
// those restrictions.
//
// Copyright 2012, Virginia Tech, University of New Mexico, and Christopher Wolfe
//
// ========================= Boost License ====================================
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// =============================== GPL V3 ====================================
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "Storage.h"
#include <linux/falloc.h>
#include <fcntl.h>
#include <errno.h>
#include "../Primitives/TypeConversion.h"

volatile unsigned Storage::n_external = 0;
volatile unsigned Storage::n_internal = 0;
volatile bool Storage::__isUp    = false;
vector<Storage*> Storage::external;
vector<Storage*> Storage::internal;
volatile bool Storage::asynchInProgress=false;
boost::thread* Storage::asynchThread=NULL;

DEFN_ENUM_STR(E_StS, E_StS_VALS);
INIT_ACCESS_MUTEX_ST(Storage);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  @@@@@    @             @                                               //
// @@      @@@@@         @@@@@  @                                          //
//  @@@@@    @     @@@@    @        @@@                                    //
//      @@   @    @@ @@    @    @  @@                                      //
//  @@@@@     @@   @@@ @    @@  @   @@@                                    //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


string Storage::humanReadable(const size_t size){
	size_t sz = size;
	size_t u  = 1ll;
	size_t i  = 0ll;
	while (sz>=1024){
		u  *= 1024ll;
		sz /= 1024ll;
		i++;
	}
	double v = (double)size/(double)u;
	string unit = "";
	switch(i){
		case 0: /*1024^0 ~=  10^0*/  unit = "B  "; break;
		case 1: /*1024^1 ~=  10^3*/  unit = "KiB"; break;
		case 2: /*1024^2 ~=  10^6*/  unit = "MiB"; break;
		case 3: /*1024^3 ~=  10^9*/  unit = "GiB"; break;
		case 4: /*1024^4 ~= 10^12*/  unit = "TiB"; break;
		case 5: /*1024^5 ~= 10^15*/  unit = "PiB"; break;
		case 6: /*1024^6 ~= 10^18*/  unit = "EiB"; break;
		case 7: /*1024^7 ~= 10^21*/  unit = "ZiB"; break;
		default: unit = "XXX"; break;
	}
	char buf[1024];
	bzero(buf,1024);
	snprintf(buf,1023,"%5.5f %s",v,unit.c_str());
	//stringstream ss;
	//ss << setprecision(5) << v << " " << unit;
	//return ss.str();
	return string(buf);
}
string Storage::humanReadableBW(const double rate){
	double r = rate;
	double u  = 1.0;
	size_t i  = 0;
	while (r>=1024){
		u  *= 1024.0;
		r /= 1024.0;
		i++;
	}
	double v = (double)rate/(double)u;
	string unit = "";
	switch(i){
		case 0: /*1024^0 ~=  10^0*/  unit = "B/s  "; break;
		case 1: /*1024^1 ~=  10^3*/  unit = "KiB/s"; break;
		case 2: /*1024^2 ~=  10^6*/  unit = "MiB/s"; break;
		case 3: /*1024^3 ~=  10^9*/  unit = "GiB/s"; break;
		case 4: /*1024^4 ~= 10^12*/  unit = "TiB/s"; break;
		case 5: /*1024^5 ~= 10^15*/  unit = "PiB/s"; break;
		case 6: /*1024^6 ~= 10^18*/  unit = "EiB/s"; break;
		case 7: /*1024^7 ~= 10^21*/  unit = "ZiB/s"; break;
		default: unit = "XXX/s"; break;
	}
	char buf[1024];
	bzero(buf,1024);
	snprintf(buf,1023,"%5.5f %s",v,unit.c_str());
	//stringstream ss;
	//ss << setprecision(5) << v << " " << unit;
	//return ss.str();
	return string(buf);
}
void Storage::reportAll(){
	SERIALIZE_ACCESS_ST();
	LOG_START_SESSION(L_INFO);
	cout << ANSI::hl("============================================================",basic,yellow,black)<<"\n";
	cout << ANSI::hl("==       Storage Subsystem Report                         ==",basic,yellow,black)<<"\n";
	cout << ANSI::hl("============================================================",basic,yellow,black)<<"\n";
	if (!__isUp){
		cout << "==                  -- " + ANSI::hl("Offline",blink,red,black) + " --                         ==\n";
	} else {
		cout << "==                  -- " + ANSI::hl("Online",bold,green,black) + " ---                         ==\n";
		cout << "== Number Internal Devices:     " << n_internal << "\n";
		for (unsigned i=0; i<n_internal; i++){
			cout <<"== Internal device "<<i<<"\n";
			internal[i]->report();
		}
		cout << "== Number External Devices:     " << n_external << "\n";
		for (unsigned i=0; i<n_external; i++){
			cout <<"== External device "<<i<<"\n";
			external[i]->report();
		}
	}
	cout << ANSI::hl("============================================================",basic,yellow,black)<<"\n";
	cout << ANSI::hl("==       End Report                                       ==",basic,yellow,black)<<"\n";
	cout << ANSI::hl("============================================================",basic,yellow,black)<<"\n";
	LOG_END_SESSION();
}

bool Storage::isUp(){
	SERIALIZE_ACCESS_ST();
	return __isUp && ! asynchInProgress;
}
StorageState Storage::getState(){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return Down;
	if (haveDevices(ST_INTERNAL)){
		if (haveDevices(ST_EXTERNAL)){
			return UpBoth;
		} else {
			return UpInternal;
		}
	} else {
		if (haveDevices(ST_EXTERNAL)){
			return UpExternal;
		} else {
			return Down;
		}
	}
}
#define DEBUG_STORAGE

void Storage::__doUp(bool rescan){
	// up storage
	LOGC(L_INFO, "Beginning storage subsystem initialization.", OBJECT_COLORS);


	if (rescan)
		Shell::run("/LWA/scripts/StorageControl.sh up",false,0);

	// scan for internal drives
	Result res_int = Shell::run("ls /LWA_STORAGE/Internal | sed 's/\\s*$//'");
	LOGC(L_DEBUG, "ls /LWA_STORAGE/Internal:\n" + res_int.output + "'", OBJECT_COLORS);
	vector<string> int_paths;
	boost::split(int_paths,res_int.output,boost::is_any_of("\n\t "));
	foreach(string path, int_paths){
		if (!path.empty()){
			LOGC(L_DEBUG, "Examining '/LWA_STORAGE/Internal/" + path + "'", OBJECT_COLORS);
			Storage* i = new Storage("/LWA_STORAGE/Internal/" + path, ST_INTERNAL, n_internal);
			if (i != NULL){
				if (i->isValid()){
					internal.push_back(i);
					n_internal ++;
				} else {
					delete i;
				}
			}
		}
	}
	// scan for external drives
	Result res_ext = Shell::run("ls /LWA_STORAGE/External | sed 's/\\s*$//'");
	//LOGC(L_DEBUG, "ls /LWA_STORAGE/Internal:\n" + res_ext.output + "'", OBJECT_COLORS);

	vector<string> ext_paths;
	boost::split(ext_paths,res_ext.output,boost::is_any_of("\n\t "));
	foreach(string path, ext_paths){
		if (!path.empty()){
			LOGC(L_DEBUG, "Examining '/LWA_STORAGE/External/" + path + "'", OBJECT_COLORS);
			Storage* i = new Storage("/LWA_STORAGE/External/" + path, ST_EXTERNAL, n_external);
			if (i != NULL){
				if (i->isValid()){
					external.push_back(i);
					n_external++;
				}else{
					delete i;
				}
			}
		}
	}
	__isUp = true;
	LOGC(L_INFO, "Storage subsystem initialized!!!", OBJECT_COLORS);
	if (asynchInProgress){
		SERIALIZE_ACCESS_ST();
		asynchInProgress=false;
	}
}

void Storage::__doDown(bool rescan){
	LOGC(L_INFO, "START DOWN", FATAL_COLORS);
	__isUp=false;
	foreach(Storage* s, external){
		if (s!=NULL) {
			LOGC(L_INFO, "DOWN EXT:" + s->volname, FATAL_COLORS);
			s->__goingDown();
			delete s;
		}
	}
	external.clear();
	n_external = 0;

	foreach(Storage* s, internal){
		if (s!=NULL) {
			LOGC(L_INFO, "DOWN INT:" + s->volname, FATAL_COLORS);
			s->__goingDown();
			delete s;
		}
	}
	internal.clear();
	n_internal = 0;
	LOGC(L_INFO, "DOWN DONEISH", FATAL_COLORS);


	if (rescan)
		Shell::run("/LWA/scripts/StorageControl.sh down",false,0);

	if (asynchInProgress){
		SERIALIZE_ACCESS_ST();
		asynchInProgress=false;
	}
}


bool Storage::up(bool rescan){
	SERIALIZE_ACCESS_ST();
	if (asynchInProgress){
		LOGC(L_INFO, "Operation pending", FATAL_COLORS);
		return false;
	}
	if (__isUp) {
		LOGC(L_INFO, "Not Down", FATAL_COLORS);
		return false;
	}
	__doUp(rescan);
	return true;
}


bool Storage::down(bool rescan){
	SERIALIZE_ACCESS_ST();
	if (asynchInProgress){
		LOGC(L_INFO, "Operation pending", FATAL_COLORS);
		return false;
	}
	if (!__isUp){
		LOGC(L_INFO, "Not Up", FATAL_COLORS);
		return false;
	}
	__doDown(rescan);
	return true;
}

bool Storage::upAsynch(bool rescan){
	SERIALIZE_ACCESS_ST();
	if (asynchInProgress){
		LOGC(L_INFO, "Operation pending", FATAL_COLORS);
		return false;
	}
	if (__isUp){
		LOGC(L_INFO, "Not Down", FATAL_COLORS);
		return false;
	}
	asynchInProgress=true;
	if (asynchThread != NULL){
		asynchThread->join();
		delete(asynchThread);
	}
	asynchThread = new boost::thread(Storage::__doUp,rescan);
	if (!asynchThread){																									\
		LOGC(L_ERROR, "[Storage] : failed to create asynchronous thread", SECURITY_COLORS);
		asynchInProgress = false;
		return false;
	}
	return true;
}

bool Storage::downAsynch(bool rescan){
	SERIALIZE_ACCESS_ST();
	if (asynchInProgress){
		LOGC(L_INFO, "Operation pending", FATAL_COLORS);
		return false;
	}
	if (!__isUp){
		LOGC(L_INFO, "Not Up", FATAL_COLORS);
		return false;
	}
	asynchInProgress=true;
	if (asynchThread != NULL){
		asynchThread->join();
		delete(asynchThread);
	}
	asynchThread = new boost::thread(Storage::__doDown,rescan);
	if (!asynchThread){																									\
		LOGC(L_ERROR, "[Storage] : failed to create asynchronous thread", SECURITY_COLORS);
		asynchInProgress = false;
		return false;
	}
	return true;
}

bool     Storage::haveDevices(StorageType st){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return false;
	switch(st){
		case ST_INTERNAL:   return n_internal != 0;
		case ST_EXTERNAL:	return n_external != 0;
		case ST_NETWORK:	// fall through
		default: return false;
	}
}
unsigned Storage::getDeviceCount(StorageType st){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return 0;
	switch(st){
		case ST_INTERNAL:   return n_internal;
		case ST_EXTERNAL:	return n_external;
		case ST_NETWORK:	// fall through
		default: return 0;
	}
}

int Storage::resolveArraySelect(StorageType st, string as, string& comment){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress){
		return -1;
	}
	if (as.empty())
		return -1;
	Storage*s;
	string allAfterFirst = string(&(as.c_str()[1]));
	string typestring;
	switch(st){
		case ST_INTERNAL:   typestring="DRSU";             break;
		case ST_EXTERNAL:	typestring="External Drive";   break;
		case ST_NETWORK:	typestring="Network Resource"; break;
		default:            typestring="Unknown Resource"; break;
	}
	if (as[0] == ')') {
		// by name
		LOGC(L_DEBUG, "RESOLVE BY NAME", STORAGE_COLORS);
		s=getDevice(allAfterFirst,st);
		comment = "Unknown "+typestring+" volume name '"+allAfterFirst+"'";
	} else if (as[0] == ']') {
		// by dev id
		LOGC(L_DEBUG, "RESOLVE BY DEV ID", STORAGE_COLORS);
		s=getDeviceByPartition(allAfterFirst,st);
		comment = "Unknown "+typestring+" partition name '"+allAfterFirst+"'";
	} else if (as[0] == '>') {
		// by mountpoint
		LOGC(L_DEBUG, "RESOLVE BY DEV MP", STORAGE_COLORS);
		s=getDeviceByMountPoint(allAfterFirst,st);
		comment = "Unknown "+typestring+" mount point name '"+allAfterFirst+"'";
	} else {
		// numeric Note: indices are 0-based, drive numbers 1-based
		LOGC(L_DEBUG, "RESOLVE BY INDEX", STORAGE_COLORS);
		unsigned id = strtoul(as.c_str(),NULL,10)-1;
		s=getDevice(id,st);
		comment = "Unknown "+typestring+" number "+as;
	}
	if (s==NULL){
		LOGC(L_DEBUG, "NO RESOLUTION", STORAGE_COLORS);
		return -1;
	} else {
		LOGC(L_DEBUG, "RESOLVED TO " + LXS(s->getId()), STORAGE_COLORS);
		comment="";
		return s->getId();
	}
}

Storage* Storage::getDevice(unsigned index, StorageType st){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return NULL;
	switch(st){
		case ST_INTERNAL:
			if (index < n_internal)
				return internal[index];
			else
				return NULL;
		case ST_EXTERNAL:
			if (index < n_external)
				return external[index];
			else
				return NULL;
		case ST_NETWORK:	// fall through
		default: return NULL;
	}
}

Storage* Storage::getDevice(string volname, StorageType st){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return NULL;

	switch(st){
		case ST_INTERNAL:   {
			for (unsigned i =0; i<n_internal; i++){
				Storage *s = internal[i];
				if (s){
					if (!s->getVolumeName().compare(volname)){
						return s;
					}
				}
			}
			return NULL;
		}
		case ST_EXTERNAL:{
			for (unsigned i =0; i<n_external; i++){
				Storage *s = external[i];
				if (s){
					if (!s->getVolumeName().compare(volname)){
						return s;
					}
				}
			}
			return NULL;
		}
		case ST_NETWORK:	// fall through
		default: return NULL;
	}
}
Storage* Storage::getDeviceByPartition(string partition, StorageType st){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return NULL;

	switch(st){
		case ST_INTERNAL:   {
			for (unsigned i =0; i<n_internal; i++){
				Storage *s = internal[i];
				if (s){
					if (!s->getPartition().compare(partition)){
						return s;
					}
				}
			}
			return NULL;
		}
		case ST_EXTERNAL:{
			for (unsigned i =0; i<n_external; i++){
				Storage *s = external[i];
				if (s){
					if (!s->getPartition().compare(partition)){
						return s;
					}
				}
			}
			return NULL;
		}
		case ST_NETWORK:	// fall through
		default: return NULL;
	}
}
Storage* Storage::getDeviceByMountPoint(string mountpoint, StorageType st){
	SERIALIZE_ACCESS_ST();
	if (!__isUp || asynchInProgress) return NULL;

	switch(st){
		case ST_INTERNAL:   {
			for (unsigned i =0; i<n_internal; i++){
				Storage *s = internal[i];
				if (s){
					if (!s->getMountPoint().compare(mountpoint)){
						return s;
					}
				}
			}
			return NULL;
		}
		case ST_EXTERNAL:{
			for (unsigned i =0; i<n_external; i++){
				Storage *s = external[i];
				if (s){
					if (!s->getMountPoint().compare(mountpoint)){
						return s;
					}
				}
			}
			return NULL;
		}
		case ST_NETWORK:	// fall through
		default: return NULL;
	}
}




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//  @@@@@  @          @@@    @@@                                           //
// @@      @         @@     @@                                             //
// @       @   @@@@   @@@    @@@                                           //
// @@      @  @@ @@     @@     @@                                          //
//  @@@@@   @  @@@ @ @@@@   @@@@                                           //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

// information about the filesystem
	bool        Storage::isValid()        const { return valid;      }
	StorageType Storage::getStorageType() const {return __type;      }
	int         Storage::getId()          const { return id;         }
	string      Storage::getMountPoint()  const { return path;       }
	string      Storage::getPartition()   const { return partition;  }
	string      Storage::getVolumeName()  const { return volname;    }
	size_t      Storage::getBlockSize()   const { return blockSize;  }
	size_t      Storage::getBlockCount()  const { return blockCount; }
	size_t      Storage::getSize()        const { return size;       }
	size_t      Storage::getRaidDrives()  const { return driveCount; }
	size_t      Storage::getRaidStripe()  const { return stripe;     }
	size_t      Storage::getRaidStride()  const { return stride;     }

	size_t Storage::getFreespace()  const {
		SERIALIZE_ACCESS();
		struct statvfs vstat;
		if ((statvfs( path.c_str(), &vstat ) == 0) && (vstat.f_favail))
			return vstat.f_bavail * vstat.f_bsize;
		else
			return 0;
	}

	// information about contents
	int Storage::getFileCount(FileType t){
		SERIALIZE_ACCESS();
		size_t i = (size_t) t;
		if (i >= folders.size())
			return -1;
		else {
			if(folders[t]){
				return folders[t]->getFileCount();
			} else {
				return -1;
			}
		}
	}

	string Storage::getFileName(int id, FileType t){
		SERIALIZE_ACCESS();
		size_t i = (size_t) t;
		if (i >= folders.size())
			return "/";
		else {
			if(folders[t]){
				return folders[t]->getFileName(id);
			}
		}
		return "/";
	}

	Storage::~Storage(){
		if (valid){
			LOG_START_SESSION(L_DEBUG);
			cout << "Closed storage volume"                << endl;
			cout << "\tType               " << storageTypeString(__type) << endl;
			cout << "\tPartition          " << partition   << endl;
			cout << "\tMountpoint         " << path        << endl;
			cout << "\tVolume Name        " << volname     << endl;
			LOG_END_SESSION();
			switch (__type){
			case ST_INTERNAL:
				LOGC(L_INFO, "DOWN FT_GENERAL", FATAL_COLORS);
				if(folders[FT_GENERAL])      delete folders[FT_GENERAL];folders[FT_GENERAL]=NULL;
				LOGC(L_INFO, "DOWN FT_SPECTROMETER", FATAL_COLORS);
				if(folders[FT_SPECTROMETER]) delete folders[FT_SPECTROMETER];folders[FT_SPECTROMETER]=NULL;
				break;
			case ST_EXTERNAL:
				if(folders[FT_GENERAL])      delete folders[FT_GENERAL];folders[FT_GENERAL]=NULL;
				break;
			case ST_NETWORK:
				break;
			}
		}

	}
Storage::Storage(string path, StorageType type, int id):
		id(id),
		path(path),
		partition(""),
		volname(""),
		blockSize(0),
		blockCount(0),
		stripe(0),
		stride(0),
		size(0),
		driveCount(0),
		__type(type),
		valid(false),
		folders((__type == ST_INTERNAL) ? 2 : 1),
		activeFiles(){
	// check path exists and is mount point
	Result mp_vc = Shell::run("if mountpoint -q \""+path+"\"; then echo -n 'valid'; else echo -n 'invalid'; fi", false, 0);
	if (!mp_vc.output.compare("invalid")){
		// either doesn't exist or is not a mountpoint
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: path '"+path+"' is not a mountpoint. '"<<mp_vc.output<<"' \n";
		LOG_END_SESSION();
		return;
	}

	// get partition
	Result par = Shell::run("mount | grep " + path + _first_word + _strip_newlines, false, 0);
	partition = par.output;

	// check partition returned is valid block device
	Result par_is_block = Shell::run("if [ -b '" + partition + "' ]; then echo -n 'valid'; else echo -n 'invalid'; fi", false, 0);
	if (!par_is_block.output.compare("invalid")){
		// either doesn't exist or is not a block device
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: not a block device. '"<<par_is_block.output<<"' \n";
		LOG_END_SESSION();

		return;
	}

	// re-validate formatting options for internal storage
	if (__type == ST_INTERNAL){
		Result doublecheck = Shell::run("/LWA/scripts/StorageControl.sh check " + partition , false, 0);
		if (!doublecheck.output.compare("invalid")){
			// either doesn't exist or is not a block device or is not formatted correctly
			LOG_START_SESSION(L_DEBUG);
			cout << "Warning: double check failed. '"<<doublecheck.output<<"' \n";
			LOG_END_SESSION();
			return;
		}
	}

	__createFileStructure();

	// at this point, we're valid, but need to get some info before returning
	// we could get all this through statvfs, but it's just as easy to use tune2fs and these won't get called often
	Result bs  = Shell::run("tune2fs -l " + partition + " | grep 'Block size'" + _last_word + _strip_newlines ,false,0);
	blockSize  = strtoul(bs.output.c_str(),NULL,10);
	Result bc  = Shell::run("tune2fs -l " + partition + " | grep 'Block count'" + _last_word + _strip_newlines ,false,0);
	blockCount = strtoul(bc.output.c_str(),NULL,10);
	size = blockSize * blockCount;

	Result sc  = Shell::run("echo \"0`tune2fs -l " + partition + " | grep stride" +_last_word + _strip_newlines +  "` + 0\" | bc" + _strip_newlines,false,0);
	stride     = blockSize * strtoul(sc.output.c_str(),NULL,10);
	Result ss  = Shell::run("echo \"0`tune2fs -l " + partition + " | grep stripe" +_last_word + _strip_newlines +  "` + 0\" | bc" + _strip_newlines,false,0);
	stripe     = blockSize * strtoul(ss.output.c_str(),NULL,10);

	if (stripe && stride)
		driveCount = stripe/stride;
	else
		driveCount = 1;

	Result vn  = Shell::run("blkid -s LABEL -o value " + partition + _strip_newlines, false, 0);
	if (vn.output.size()==0)
		volname = "Nothing Assigned";
	else
		volname = vn.output;

	switch (__type){
	case ST_INTERNAL:
		folders[FT_GENERAL]      = new Folder(path + "/DROS/Rec" ,  FT_GENERAL);
		folders[FT_SPECTROMETER] = new Folder(path + "/DROS/Spec" , FT_SPECTROMETER);
		break;
	case ST_EXTERNAL:
		folders[FT_GENERAL]      = new Folder(path + "/DROS/" , FT_GENERAL);
		break;
	case ST_NETWORK:
		break;
	}

	LOG_START_SESSION(L_DEBUG);
	cout << "Opened storage volume"                << endl;
	cout << "\tType               " << storageTypeString(__type) << endl;
	cout << "\tPartition          " << partition   << endl;
	cout << "\tMountpoint         " << path        << endl;
	cout << "\tVolume Name        " << volname     << endl;
	cout << "\tBlock Size         " << humanReadable(blockSize) << endl;
	cout << "\tBlock Count        " << blockCount  << endl;
	cout << "\tSize               " << humanReadable(size) << endl;
	cout << "\tNumber Drives      " << driveCount  << endl;
	cout << "\tRaid Stride Size   " << humanReadable(stride)  << endl;
	cout << "\tRaid Stripe Size   " << humanReadable(stripe)  << endl;
	LOG_END_SESSION();
	valid = true;
}



Storage::Storage(){
	/* do nothing */
}
Storage::Storage(const Storage& toCopy){
	/* do nothing */
}

void Storage::__createFileStructure(){
	switch (__type){
	case ST_INTERNAL:
		Shell::run("mkdir -p " + path + "/DROS", false,  0);
		Shell::run("mkdir -p " + path + "/DROS/Rec", false,  0);
		Shell::run("mkdir -p " + path + "/DROS/Spec", false, 0);
		break;
	case ST_EXTERNAL:
		Shell::run("mkdir -p " + path + "/DROS", false,  0);
		break;
	case ST_NETWORK:
		break;
	}
}

void Storage::__deleteAll(){
	switch (__type){
	case ST_INTERNAL:
		Shell::run("rm -rf " + path + "/DROS/Rec/*", false,  0);
		Shell::run("rm -rf " + path + "/DROS/Spec/*", false, 0);
		break;
	case ST_EXTERNAL:
		Shell::run("rm -rf " + path + "/DROS/*", false,  0);
		break;
	case ST_NETWORK:
		break;
	}
}
void  Storage::formatDevice(){
	__format();
}

bool  Storage::setVolumeName(const string & newName){
	SERIALIZE_ACCESS();
	if (strchr(newName.c_str(),'\'') != NULL) return false;
	if (newName.size() > 16) return false;
	string cmd = "x=`tune2fs -L '" + newName + "' " + getPartition()  + " 2>&1 > /dev/null && echo valid || echo invalid`; echo $x " + _strip_newlines;
	Result bs  = Shell::run(cmd,false,0);
	if (!bs.output.compare("valid")){
		string ignored;
		string as = CONF_GET(ArraySelect);
		if ((getStorageType()==ST_INTERNAL)&&(resolveArraySelect(ST_INTERNAL, as,ignored) == getId())){
			CONF_SET(ArraySelect,")"+newName);
		}
		volname = newName;
		return true;
	} else {
		return false;
	}
}

void Storage::__format(){
	__deleteAll();
	__createFileStructure();
}

void Storage::__goingDown(){
	foreach(File* f, activeFiles){
		if (f!=NULL) {
			if (f->isOpen()){
				f->close();
				f->setAttribute("howClosed","Storage::goingDown");
			}
		}
	}
}

File* Storage::getOutFile(FileType type, string name, size_t preallocate){
	SERIALIZE_ACCESS();
	Folder* fo = NULL;
	size_t i = (size_t) type;
	if ((i >= folders.size()) || folders[type]==NULL){
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: request output file of unsupported type ("<<fileTypeString(type)<<") on this device ("<<partition<<").\n";
		LOG_END_SESSION();
		return NULL;
	} else {
		fo = folders[type];
	}
	string fullname = fo->path + "/" + name;
	int fd = open(
			fullname.c_str(),
			O_RDWR | O_CREAT | O_APPEND | O_EXCL,
			S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP
	);
	if (fd==-1){
		LOG_START_SESSION(L_DEBUG);
		cout << "Error:cannot create file '"<<fullname<<"' on this device ("<<partition<<"); Reason: '"<<strerror(errno)<<"'.\n";
		LOG_END_SESSION();
		return NULL;
	}
	int res;
	if (preallocate != 0){
		res=fallocate64(fd, FALLOC_FL_KEEP_SIZE, 0, preallocate);
		if (res<0){
			LOG_START_SESSION(L_DEBUG);
			cout << "Warning: cannot preallocate " << humanReadable(preallocate) << "for file '"<<fullname<<"' on this device ("<<partition<<"); Reason: '"<<strerror(errno)<<"'.\n";
			LOG_END_SESSION();
		}
	}
	res = posix_fadvise64(fd, 0,0,POSIX_FADV_SEQUENTIAL);
	if (res<0){
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: cannot advise kernel of file usage mode for file '"<<fullname<<"' on this device ("<<partition<<"); Reason: '"<<strerror(errno)<<"'.\n";
		LOG_END_SESSION();
	}
	File* f = new File(name,fullname,__type,type,fd);
	if (f == NULL){
		LOG_START_SESSION(L_DEBUG);
		cout << "Error:cannot allocate file object for file '"<<fullname<<"' on this device ("<<partition<<").\n";
		LOG_END_SESSION();
		::close(fd);
		return NULL;
	} else {
		f->setAttribute("preallocate",LXS(preallocate));
		f->setAttribute("howOpened","Storage::getOutFile");
		activeFiles.insert(f);
		return f;
	}
}

File* Storage::getInFile(FileType type, string name){
	SERIALIZE_ACCESS();
	Folder* fo = NULL;
	size_t i = (size_t) type;
	if ((i >= folders.size()) || folders[type]==NULL){
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: request output file of unsupported type ("<<fileTypeString(type)<<") on this device ("<<partition<<").\n";
		LOG_END_SESSION();
		return NULL;
	} else {
		fo = folders[type];
	}

	string fullname = fo->path + "/" + name;
	int fd = open(fullname.c_str(),	O_RDONLY);
	if (fd==-1){
		LOG_START_SESSION(L_DEBUG);
		cout << "Error:cannot open file '"<<fullname<<"' on this device ("<<partition<<"); Reason: '"<<strerror(errno)<<"'.\n";
		LOG_END_SESSION();
		return NULL;
	}
	int res;
	res = posix_fadvise64(fd, 0,0,POSIX_FADV_SEQUENTIAL);
	if (res<0){
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: cannot advise kernel of file usage mode for file '"<<fullname<<"' on this device ("<<partition<<"); Reason: '"<<strerror(errno)<<"'.\n";
		LOG_END_SESSION();
	}
	File* f = new File(name,fullname,__type,type,fd);
	if (f == NULL){
		LOG_START_SESSION(L_DEBUG);
		cout << "Error:cannot allocate file object for file '"<<fullname<<"' on this device ("<<partition<<").\n";
		LOG_END_SESSION();
		::close(fd);
		return NULL;
	} else {
		f->setAttribute("howOpened","Storage::getInFile");
		activeFiles.insert(f);
		return f;
	}


}
File* Storage::getQueryFile(FileType type, string name){
	SERIALIZE_ACCESS();
	Folder* fo = NULL;
	size_t i = (size_t) type;
	if ((i >= folders.size()) || folders[type]==NULL){
		LOG_START_SESSION(L_DEBUG);
		cout << "Warning: request output file of unsupported type ("<<fileTypeString(type)<<") on this device ("<<partition<<").\n";
		LOG_END_SESSION();
		return NULL;
	} else {
		fo = folders[type];
	}

	string fullname = fo->path + "/" + name;
	File* f = new File(name,fullname,__type,type,-1);
	if (f == NULL){
		LOG_START_SESSION(L_DEBUG);
		cout << "Error:cannot allocate query file object for file '"<<fullname<<"' on this device ("<<partition<<").\n";
		LOG_END_SESSION();
		return NULL;
	} else {
		activeFiles.insert(f);
		return f;
	}
}
File* Storage::getQueryFile(FileType type, int k){

	//k is -n..-1 or 0..(n-1)

	SERIALIZE_ACCESS();
	Folder* fo = NULL;
	size_t i = (size_t) type;
	if ((i >= folders.size()) || folders[type]==NULL){
		LOGC(L_ERROR, "Request query file of unsupported type '" + fileTypeString(type) + "' on this device (" + partition+").", OBJECT_ERROR_COLORS);
		return NULL;
	} else {
		fo = folders[type];
	}

	string name = fo->getFileName(k);
	if (name.empty()){
		LOGC(L_WARNING, "Request index "+LXS(k)+" does not exist.", OBJECT_ERROR_COLORS);
		return NULL;
	} else {
		string fullname = fo->path + "/" + name;
		File* f = new File(name,fullname,__type,type,-1);
		if (f == NULL){
			LOGC(L_ERROR, "Cannot allocate query file object for file '" + fullname + "' on this device (" + partition+").", OBJECT_ERROR_COLORS);
			return NULL;
		} else {
			activeFiles.insert(f);
			return f;
		}
	}

}

void  Storage::putFile(File* file){
	SERIALIZE_ACCESS();
	if (file){
		if (file->isOpen()){
			file->close();
			file->setAttribute("howClosed","Storage::putFile");
		}
		activeFiles.erase(file);
		delete file;
	}
}

unsigned Storage::getActiveFilecount(){
	SERIALIZE_ACCESS();
	return activeFiles.size();
}

void Storage::report(){
	SERIALIZE_ACCESS();
	LOG_START_SESSION(L_DEBUG);
	cout << "==   Type               " << ANSI::hl(storageTypeString(__type),basic,green,black) << endl;
	cout << "==   Partition          " << ANSI::hl(partition,underline,red,black)   << endl;
	cout << "==   Mountpoint         " << path        << endl;
	cout << "==   Volume Name        " << ANSI::hl(volname,basic,red,black)     << endl;
	cout << "==   Block Size         " << humanReadable(blockSize) << endl;
	cout << "==   Block Count        " << blockCount               << endl;
	cout << "==   Size               " << humanReadable(size)      << endl;
	cout << "==   Number Drives      " << driveCount               << endl;
	cout << "==   Raid Stride Size   " << humanReadable(stride)    << endl;
	cout << "==   Raid Stripe Size   " << humanReadable(stripe)    << endl;
	cout << "==   Monitored folders  " << folders.size()           << endl;
	int fi=0;
	foreach(Folder* f, folders){
		cout << "==    * Monitored folder "<<fi<<"   (Purpose: "<<fileTypeString(f->getFileType())<<")\n";
		cout << "==      " << ANSI::hl(f->getPath(),underline,yellow,black) << endl;
		fi++;
		unsigned nd = f->getDirCount();
		for (unsigned i=0; i<nd; i++)
			cout << "==       ** SubDir "<<setw(4)<<i<<":\t'" << f->getDirName(i) << "'\n";
		unsigned nf = f->getFileCount();
		for (unsigned i=0; i<nf; i++){
			string fn = f->getFileName(i);
			File* fi = getQueryFile(f->getFileType(),fn);
			if (fi){
				size_t alloc = fi->getAllocatedSize();
				size_t size  = fi->getSize();
				cout << "==       ** File   "<<setw(4)<<i<<":\t'" << fn << "' "<<humanReadable(size)<<" ("<<humanReadable(alloc)<<" allocated)\n";
				putFile(fi);
			} else {
				cout << "==       ** File   "<<setw(4)<<i<<":\t'" << fn << "' ??? bytes (??? bytes allocated)\n";
			}
		}
		cout << "==\n";
	}
	cout << "==   Active Files       " << activeFiles.size() << endl;
	int i=0;
	foreach(File* f, activeFiles){
		cout << "==    * Active File " << setw(4) << i << ":\t" << ANSI::hl(f->getFullPath(),underline,yellow,black) << " [" << fileTypeString(f->getFileType()) <<"]"<<endl;
		i++;
	}
	cout << "==  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ==\n";
	cout << "==\n";
	LOG_END_SESSION();

}
