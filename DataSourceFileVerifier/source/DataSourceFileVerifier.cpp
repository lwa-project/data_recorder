//============================================================================
// Name        : DataSourceFileVerifier.cpp
// Author      : Christopher Wolfe
// Version     :
// Copyright   : (C) Virginia Tech, MPRG  2009
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
using namespace std;

int main(int argc, char* argv[]) {
	size_t packetsize=strtoul(argv[1], NULL, 10);
	FILE * f = fopen(argv[2],"rb");
	if (!f) {
		cout << "Could not open '" << argv[2] << "'\n";
		return -1;
	}
	size_t filesize=0;
	size_t result=0;
	char * buf = (char*)malloc (packetsize);
	if (!buf){
		cout << "Could allocate buffer of size '" << argv[1] << "' bytes\n";
		return -1;
	}
	size_t* tags=(size_t *) buf;
	size_t br = 0  ;
	size_t ts  ;
	size_t sid ;
	size_t key ;
	size_t l_ts  = 0;
	size_t l_sid = 0;
	size_t l_key = 0;
	int init = 0;
	size_t pid=0;
	unsigned int i;
	size_t warnings =0;

	fseek (f , 0 , SEEK_END);
	filesize = ftell (f);
	rewind (f);
	if (((filesize / packetsize) * packetsize) != filesize){
		cout << "Warning: file size is not an integral multiple of packet size\n";
	}
	cout << "Beginning verification of '" << argv[2] << "'\n";
	while (!feof(f)) {
		result = fread (buf,1,packetsize,f);
		if (result != packetsize) {
			if (br!=filesize)
			cout << "Warning: read " << result << " bytes  when expecting " << packetsize << " bytes\n";
	    } else {
	    	ts  = tags[0];
	    	sid = tags[1];
	    	key = tags[2];
	    	if (!init){
	    		init=1;
	    	} else {
	    		if (ts < l_ts) {
	    			cout << "Warning (@ " << pid << "): Non-increasing timestamps : " << l_ts << " ==> " << ts << "\n";
	    			warnings++;
	    		}
	    		if (key != l_key) {
	    			cout << "Warning (@ " << pid << "): Key changed : " << l_key << " ==> " << key << "\n";
	    			warnings++;
	    		}
	    		if (sid != (l_sid+1)) {
	    			cout << "Warning (@ " << pid << "): Unexpected serial id sequence : " << l_sid << " ==> " << sid << "\n";
	    			warnings++;
	    		}
	    	}
    		l_ts=ts;	l_sid=sid;	l_key=key;
    		for (i = (3 * sizeof(size_t)); i<packetsize; i++ ){
				if (buf[i]!=(char)(i&0xff)){
					cout << "Error (@ " << pid << ", byte " << i << "): Byte mismatch : " << ((int)buf[i]) << " ==> " << (i&0xff) << "\n";
					fclose (f);
					cout << "Verification of '" << argv[2] << "' failed with " << warnings << " warnings\n";
					return -1;
				}
			}
	    }
		br+=result;
		pid++;
	}

	fclose (f);
	cout << "Completed verification of '" << argv[2] << "' successfully with " << warnings << " warnings\n";
	return 0;
}
