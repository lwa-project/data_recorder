// Example for C++ Interface to Gnuplot

// requirements:
// * gnuplot has to be installed (http://www.gnuplot.info/download.html)
// * for Windows: set Path-Variable for Gnuplot path (e.g. C:/program files/gnuplot/bin)
//             or set Gnuplot path with: Gnuplot::set_GNUPlotPath(const std::string &path);


#include <iostream>
#include <sstream>
#include <stdint.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include "gnuplot_i.hpp" //Gnuplot class handles POSIX-Pipe-communikation with Gnuplot
#include "Complex.h"

void wait_for_key(); // Programm halts until keypress

using namespace std;

#define NUM_TUNINGS 2
#define NUM_POLS    2
#define NUM_STREAMS @
#define DP_BASE_FREQ_HZ         196e6
#define DRX_SAMPLES_PER_FRAME   4096
#define FREQ_CODE_FACTOR        (DP_BASE_FREQ_HZ / (double)0x100000000)

typedef struct __DrxXcpResultsHeader{
	uint32_t			MAGIC1;     // must always equal 0xC0DEC0DE
	uint64_t 			timeTag0;
	uint16_t 			timeOffset;
	uint16_t 			decFactor;
	uint32_t 			freqCode[NUM_TUNINGS];
	uint8_t 			beam;
	uint32_t 			nSamplesPerOutputFrame;
	uint32_t 			nInts;
	uint32_t 			satCount;
	uint32_t			MAGIC2;     // must always equal 0xED0CED0C
} __attribute__((packed)) DrxXcpResultsHeader;

#define COUNT 128

ComplexType* data[NUM_TUNINGS];
ComplexType* dataTemp;
int fd;
uint32_t nSpf;
uint32_t nI;
DrxXcpResultsHeader hdr;
DrxXcpResultsHeader hdrTmp;
uint64_t numResults;
uint64_t start;
uint64_t length;
uint64_t ts0;
//double* timeSeriesTotalPower[NUM_TUNINGS];
//double* integratedSpectra[NUM_TUNINGS];

double*  timeStamp;
double*  realpart[NUM_TUNINGS];
double*  imagpart[NUM_TUNINGS];
double*  magnitude[NUM_TUNINGS];
double*  angle[NUM_TUNINGS];
double*  satCount;




double freq1;
double freq2;
double bandwidth;
bool printFirstFrame=false;

void printResultsHeader(){
	cout << "=============================================\n";
	cout << "MAGIC1      " << hex << hdrTmp.MAGIC1            << endl;
	cout << "timeTag0    " << dec << hdrTmp.timeTag0;
	uint64_t tt = hdrTmp.timeTag0/196000l;
	uint64_t y =   tt/(365l * 24l * 60l * 60l * 1000l);			tt = tt - (y * (365l * 24l * 60l * 60l * 1000l));
	uint64_t m =   tt/(30l * 24l * 60l * 60l * 1000l);			tt = tt - (m * (30l * 24l * 60l * 60l * 1000l));
	uint64_t d =   tt/(24l * 60l * 60l * 1000l);      			tt = tt - (d * (24l * 60l * 60l * 1000l));
	uint64_t hh =  tt/(60l * 60l * 1000l);      			    tt = tt - (hh * (60l * 60l * 1000l));
	uint64_t MM =  tt/(60l * 1000l);      			    		tt = tt - (MM * (60l * 1000l));
	uint64_t ss =  tt/(1000l);      			    			tt = tt - (ss * 1000l);
	uint64_t ms =  tt;
	cout << dec<< " = " << (y+1970) << "/" << (m+1) << "/" << d << "  " << hh << ":" << MM << ":" << ss << "." << ms << endl;

	cout << "timeOffset  " << dec << hdrTmp.timeOffset        << endl;
	cout << "decFactor   " << dec << hdrTmp.decFactor         << endl;
	cout << "freqCode[1] " << dec << hdrTmp.freqCode[0]       << endl;
	cout << "freqCode[2] " << dec << hdrTmp.freqCode[1]       << endl;
	cout << "beam        " << hex << (int) hdrTmp.beam        << endl;
	cout << "# / Frame   " << dec << hdrTmp.nSamplesPerOutputFrame  << endl;
	cout << "nInts       " << dec << hdrTmp.nInts             << endl;
	cout << "MAGIC2      " << hex << hdrTmp.MAGIC2            << endl;
	cout << "=============================================\n";

}
void printResults(int stream, int index, int count){
	for (int j=index; j<index+count; j++){
		cout << j << dec << " :  ";
		for (int i=0; i<(int)nSpf; i++){
			double re = data[stream][(j*nSpf) + i][0];
			double im = data[stream][(j*nSpf) + i][1];
			double aim = abs(im);
			cout << re ;
			if (im>=0)
				cout << " + ";
			else
				cout << " - ";
			cout << aim << "i  " << dec;
		}
		cout << endl;
	}
}

void readHeader(){
	ssize_t res=-1;
	res=read(fd,(void*)&hdrTmp,sizeof(DrxXcpResultsHeader));
	if (res != sizeof(DrxXcpResultsHeader)){
		cerr << "Last read (header) reported " << res << "bytes read.\n";
		close(fd);
		exit(0);
	}
}
void readData(){
	ssize_t res=-1;
	res=read(fd,(void*)dataTemp,(((size_t)nSpf) * NUM_TUNINGS * sizeof(ComplexType)));
	if ((size_t)res != (((size_t)nSpf) * NUM_TUNINGS * sizeof(ComplexType))){
		cerr << "Last read (data) reported " << res << "bytes read.\n";
		close(fd);
		exit(0);
	}
	freq1 = hdr.freqCode[0] * FREQ_CODE_FACTOR;
	freq2 = hdr.freqCode[1] * FREQ_CODE_FACTOR;
	bandwidth = DP_BASE_FREQ_HZ / hdr.decFactor;
	/*
	for(int f=0; f<(int)(nSpf * NUM_TUNINGS); f++){
		dataTemp[f] = (dataTemp[f]!=0) ? (10.0f * log10(dataTemp[f])) : 0;
	}
	*/
}

size_t getFileSize(){
	struct stat buf;
	if(fstat(fd,&buf)){
		cerr << "Can't stat file.\n";
		close(fd);
		exit(0);
	}
	return buf.st_size;
}

size_t getResultsCount(){
	return getFileSize() / (sizeof(DrxXcpResultsHeader) + (((size_t)nSpf) * NUM_TUNINGS * sizeof(ComplexType)));
}

void readResults(){
	readHeader();
	hdr=hdrTmp;
	nI         = hdr.nInts;
	nSpf       = hdr.nSamplesPerOutputFrame;
	numResults = getResultsCount();
	//uint64_t realStart = (start<numResults) ? start : 0;
	//uint64_t realLength = ((realStart+length) <numResults) ? length : (numResults - realStart);
	for(int str=0; str<NUM_TUNINGS; str++){
		data[str] = (ComplexType*) malloc (nSpf * numResults * sizeof(ComplexType));
		if (!data[str]) {
			cerr << "Can't malloc data area.\n";
			close(fd);
			exit(0);
		}
	}
	timeStamp = (double*) malloc(numResults * nSpf * sizeof(double));
	if (!timeStamp) { cerr << "Can't malloc data area (timestamps).\n"; close(fd); exit(0);	} else {bzero(timeStamp, numResults * nSpf * sizeof(double));}
	satCount = (double*) malloc(numResults * sizeof(double));
	if (!satCount) { cerr << "Can't malloc data area (satCount).\n"; close(fd); exit(0);	} else {bzero(satCount, numResults * sizeof(double));}


	dataTemp  = (ComplexType*) malloc (nSpf * NUM_TUNINGS * sizeof(ComplexType));
	if (!dataTemp){
		cerr << "Can't malloc data area.\n";
		close(fd);
		exit(0);
	}



	for (unsigned int i=0; i<numResults; i++){
		if (i!=0){ // skip reading the first header, since we already read it
			readHeader();
			if ( (hdrTmp.nSamplesPerOutputFrame != nSpf) || (hdrTmp.nInts != nI)){
				cerr << "Error, geometry changed mid file.\n";
				free(data[0]);free(data[1]);free(dataTemp);
				close(fd);
				exit(0);
			}
		}
		/*
		if (i<100){
			printResultsHeader();
		}*/
		if (i==0){
			ts0 = hdrTmp.timeTag0;
		}
		readData();
		for(int str=0; str<NUM_TUNINGS; str++){
			memcpy((void*)&data[str][i*nSpf], (void*)&dataTemp[str*nSpf], nSpf*sizeof(ComplexType));
		}
		double tfb = ((double)(hdrTmp.timeTag0-ts0));
		double tfs = ((double)(hdrTmp.decFactor * hdrTmp.nInts));
		satCount[i] = ((double)(hdrTmp.satCount));
		for (unsigned int j = 0; j<nSpf; j++){
			timeStamp[(i*nSpf)+j] = tfb + (j*tfs);
		}
	}
	free(dataTemp);
	for (int i=0; i<NUM_TUNINGS; i++){
		size_t nsize = nSpf * numResults * sizeof(double);
		realpart[i]  = (double*) malloc(nsize);
		imagpart[i]  = (double*) malloc(nsize);
		magnitude[i] = (double*) malloc(nsize);
		angle[i]     = (double*) malloc(nsize);
		if (!realpart[i]) {  cerr << "Can't malloc data area (realpart).\n";   close(fd); exit(0);	} else {bzero(realpart[i],  nsize);}
		if (!imagpart[i]) {  cerr << "Can't malloc data area (imagpart).\n";   close(fd); exit(0);	} else {bzero(imagpart[i],  nsize);}
		if (!magnitude[i]) { cerr << "Can't malloc data area (magnitude).\n";  close(fd); exit(0);	} else {bzero(magnitude[i], nsize);}
		if (!angle[i]) {     cerr << "Can't malloc data area (angle).\n";      close(fd); exit(0);	} else {bzero(angle[i],     nsize);}
	}
	for(int result=0; result<(int)numResults; result++){
		for (int tun=0; tun<NUM_TUNINGS; tun++){
			for (unsigned int sample=0; sample<nSpf; sample++){
				unsigned int tidx = tun;
				unsigned int sidx = nSpf*result + sample;
				double re             = (double)  data[tidx][sidx][0];
				double im             = (double)  data[tidx][sidx][1];
				realpart[tidx][sidx]  = re;
				imagpart[tidx][sidx]  = im;
				magnitude[tidx][sidx] = sqrt((re*re) + (im*im));
				angle[tidx][sidx]  = atan2(im,re);
			}
		}
	}

/*	if (printFirstFrame){
		for (int i =0; i<10; i++){
			cout << "Tuning 1: \t";
			printResults(0, i, 1);
			cout << "Tuning 2: \t";
			printResults(1, i, 1);
		}
		cout << endl;

	}*/

}


int main(int argc, char* argv[])
{

	if (argc<4) {
		cout << "Error: invalid # of args("<<argc<<"). \nUsage:\n\t";
		cout << argv[0] << " <filename> <start(1st spectra index)> <length(# spectra; -1 means until end)>\n";
		exit(-1);
	}
	fd = open(argv[1],  O_RDONLY | O_EXCL , S_IROTH | S_IRUSR |  S_IRGRP | S_IWOTH | S_IWUSR |  S_IWGRP );
	if (fd==-1){
		cout << "Can't open file.\n";
		return -1;
	}
	start = strtoul(argv[2],NULL,10);
	ssize_t  len_temp=strtol(argv[3],NULL,10);
	length = ((len_temp>=0) ? (size_t) len_temp : (size_t) -1);
	printFirstFrame = (bool)strtoul(argv[3],NULL,10);


	readResults();
	cout << "Found " <<dec<< numResults << " results of size "<< nSpf << " samples per frame (integration count = "<<nI<<" )." << endl;fflush(stdout);
	uint64_t realStart = (start<numResults) ? start : numResults;
	uint64_t realLength = ((realStart+length) <numResults) ? length : (numResults - realStart);
	if (!realLength){
		cout << "Supplied start and length are outside of detected spectra. Exitting...\n";
		return -1;
	}
	//printSpectra(tunpolidx,realStart,realLength);

	try
    {
		//timeSeriesTotalPower[i][j-start] += data[i][j*nSpf+f];
		//integratedSpectra[i][f]    += data[i][j*nSpf+f];

		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "Plotting integrated correlations....\n";
		cout << "Tuning 1 center frequency: " << ((uint64_t)freq1) << dec << " MHz.\n";
		cout << "Tuning 2 center frequency: " << ((uint64_t)freq2) << dec << " MHz.\n";
		Gnuplot plotter1(true);
		plotter1.cmd("set multiplot;");
		plotter1.cmd("set size 0.5,0.5;");

		string tnames[NUM_TUNINGS]= {"Tuning 1","Tuning 2"};
		double xmin = (double) timeStamp[0];
		double xmax = (double) timeStamp[0];
		vector<float> x[NUM_TUNINGS],y[NUM_TUNINGS];
		for (int gidx=0; gidx<4; gidx++){

			double row = (double) (gidx >> 1);
			double col = (double) (gidx  & 1);

			for (int tun=0; tun<NUM_TUNINGS; tun++){
				x[tun].clear();
				y[tun].clear();
				for(int result=0; result<(int)numResults; result++){
					if ((result >= (int)realStart) && (result < (int)realStart+(int)realLength)){
						for (unsigned int sample=0; sample<nSpf; sample++){
							unsigned int tidx = tun;
							unsigned int sidx = nSpf*result + sample;
							x[tun].push_back(timeStamp[sidx]/196e6);
							if (timeStamp[sidx]>xmax)
								xmax=timeStamp[sidx];
							//x[tun].push_back((double)sidx);
							switch(gidx){
								case 0:  y[tun].push_back(realpart[tidx][sidx]); break;
								case 1:  y[tun].push_back(imagpart[tidx][sidx]); break;
								case 2:  y[tun].push_back(magnitude[tidx][sidx]); break;
								case 3:  y[tun].push_back(angle[tidx][sidx]); break;
							}

						}
					}
				}
			}
			stringstream s_org;
			s_org << "set origin " << ((col) * 0.5) << "," << ((1-row)*0.5) << ";";
			plotter1.cmd(s_org.str());
			//plotter1.set_xrange(xmin,nSpf*numResults + nSpf -1);
			cout << "@@@@@@@@@@@@@@@@@@@@@@ XRANGE  " << xmin << " -> " << xmax << endl;
			switch(gidx){
				case 0:  plotter1.set_title("Real"); break;
				case 1:  plotter1.set_title("Imaginary"); break;
				case 2:  plotter1.set_title("Magnitude"); break;
				case 3:  plotter1.set_title("Angle"); break;
			}
			plotter1.set_xlabel("Time (s)");
			switch(gidx){
				case 0:  plotter1.set_ylabel("Real (arb.)"); break;
				case 1:  plotter1.set_ylabel("Imaginary (arb.)"); break;
				case 2:  plotter1.set_ylabel("Magnitude (arb.)"); break;
				case 3:  plotter1.set_ylabel("Angle (radians)"); break;
			}
			plotter1.set_style("lines");
			plotter1.plot_xy_many(x, y, NUM_TUNINGS, tnames);
		}
		plotter1.cmd("unset multiplot");


		cout << "********************************************************************\n";
		cout << "Plotting saturation counts....\n";
		Gnuplot plotter2(true);
		vector<double> x2,y2;
		for(int result=0; result<(int)numResults; result++){
			if ((result >= (int)realStart) && (result < (int)realStart+(int)realLength)){
				x2.push_back(timeStamp[result*nSpf]/196e6);
				y2.push_back(satCount[result]);
			}
		}
		plotter2.set_title("Saturation Count");
		plotter2.set_xlabel("Time (s)");
		plotter2.set_ylabel("Counts");
		plotter2.set_style("lines");
		plotter2.plot_xy(x2, y2);

    }

    catch (GnuplotException ge)
    {
        cout << ge.what() << endl;
    }
    return 0;
}



void wait_for_key ()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)  // every keypress registered, also arrow keys
    cout << endl << "Press any key to continue..." << endl;

    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    _getch();
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    cout << endl << "Press ENTER to continue..." << endl;

    std::cin.clear();
    std::cin.ignore(std::cin.rdbuf()->in_avail());
    std::cin.get();
#endif
    return;
}
