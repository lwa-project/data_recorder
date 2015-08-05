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
#include "gnuplot_i.hpp" //Gnuplot class handles POSIX-Pipe-communikation with Gnuplot
#include <math.h>

void wait_for_key(); // Programm halts until keypress

using namespace std;

#define NUM_TUNINGS 2
#define NUM_POLS    2
#define NUM_STREAMS (NUM_TUNINGS * NUM_POLS)
#define DP_BASE_FREQ_HZ         196e6
#define DRX_SAMPLES_PER_FRAME   4096
#define FREQ_CODE_FACTOR        (DP_BASE_FREQ_HZ / (double)0x100000000)


typedef struct __DrxSpectraHeader{
	uint32_t			MAGIC1;     // must always equal 0xC0DEC0DE
	uint64_t 			timeTag0;
	uint16_t 			timeOffset;
	uint16_t 			decFactor;
	uint32_t 			freqCode[NUM_TUNINGS];
	uint32_t			fills[NUM_STREAMS];
	uint8_t				errors[NUM_STREAMS];
	uint8_t 			beam;
	uint32_t 			nFreqs;
	uint32_t 			nInts;
	uint32_t 			satCount[NUM_TUNINGS * NUM_POLS];
	uint32_t			MAGIC2;     // must always equal 0xED0CED0C
} __attribute__((packed)) DrxSpectraHeader;

#define COUNT 128

float* data[NUM_STREAMS];
float* dataTemp;
double* fills[NUM_STREAMS];
int fd;
uint32_t nF;
uint32_t nI;
DrxSpectraHeader hdr;
DrxSpectraHeader hdrTmp;
uint64_t numSpectra;
uint64_t start;
uint64_t length;
double* timeSeriesTotalPower[NUM_STREAMS];
double* integratedSpectra[NUM_STREAMS];
double*  satCount[NUM_STREAMS];

double freq1;
double freq2;
double bandwidth;

void printSpectraHeader(){
	cout << "=============================================\n";
	cout << "MAGIC1      " << hex << hdr.MAGIC1            << endl;
	cout << "timeTag0    " << dec << hdr.timeTag0          << endl;
	cout << "timeOffset  " << dec << hdr.timeOffset        << endl;
	cout << "decFactor   " << dec << hdr.decFactor         << endl;
	cout << "freqCode[1] " << dec << hdr.freqCode[0]       << endl;
	cout << "freqCode[2] " << dec << hdr.freqCode[1]       << endl;
	cout << "fill[0,X]   " << dec << hdr.fills[0]          << endl;
	cout << "fill[0,Y]   " << dec << hdr.fills[1]          << endl;
	cout << "fill[1,X]   " << dec << hdr.fills[2]          << endl;
	cout << "fill[1,Y]   " << dec << hdr.fills[3]          << endl;
	cout << "error[0,X]  " << dec << (int) hdr.errors[0]   << endl;
	cout << "error[0,Y]  " << dec << (int) hdr.errors[1]   << endl;
	cout << "error[1,X]  " << dec << (int) hdr.errors[2]   << endl;
	cout << "error[1,Y]  " << dec << (int) hdr.errors[3]   << endl;
	cout << "beam        " << hex << (int) hdr.beam        << endl;
	cout << "nFreqs      " << dec << hdr.nFreqs            << endl;
	cout << "nInts       " << dec << hdr.nInts             << endl;
	cout << "MAGIC2      " << hex << hdr.MAGIC2            << endl;
	cout << "=============================================\n";
}
void printSpectra(int stream, int index, int count){
	for (int j=index; j<index+count; j++){
		cout << j << dec << " :  ";
		for (int i=0; i<(int)nF; i++){
			cout << data[stream][(j*nF) + i] << " ";
		}
		cout << endl;
	}
}

void readHeader(){
	ssize_t res=-1;
	res=read(fd,(void*)&hdrTmp,sizeof(DrxSpectraHeader));
	if (res != sizeof(DrxSpectraHeader)){
		cerr << "Last read (header) reported " << res << "bytes read.\n";
		close(fd);
		exit(0);
	}
}
void readData(){
	ssize_t res=-1;
	res=read(fd,(void*)dataTemp,(nF * NUM_STREAMS * sizeof(float)));
	if ((size_t)res != (nF * NUM_STREAMS * sizeof(float))){
		cerr << "Last read (data) reported " << res << "bytes read.\n";
		close(fd);
		exit(0);
	}
	freq1 = hdr.freqCode[0] * FREQ_CODE_FACTOR;
	freq2 = hdr.freqCode[1] * FREQ_CODE_FACTOR;
	bandwidth = DP_BASE_FREQ_HZ / hdr.decFactor;
	for(int f=0; f<(int)(nF * NUM_STREAMS); f++){
		dataTemp[f] = (dataTemp[f]!=0) ? (10.0f * log10(dataTemp[f])) : 0;
	}
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

size_t getSpectraCount(){
	return getFileSize() / (sizeof(DrxSpectraHeader) + (((size_t)nF) * NUM_STREAMS * sizeof(float)));
}

void readSpectra(){
	readHeader();
	hdr=hdrTmp;
	nI         = hdr.nInts;
	nF         = hdr.nFreqs;
	numSpectra = getSpectraCount();
	uint64_t realStart = (start<numSpectra) ? start : 0;
	uint64_t realLength = ((realStart+length) <numSpectra) ? length : (numSpectra - realStart);
	for(int str=0; str<NUM_STREAMS; str++){
		fills[str]= (double*) malloc (numSpectra * sizeof(double));
		data[str] = (float*) malloc (nF * numSpectra * sizeof(float));
		satCount[str]=(double*) malloc(numSpectra * sizeof(double));
		bzero(satCount[str],numSpectra*sizeof(double));
		if (!data[str] || !fills[str]){
			cerr << "Can't malloc data area.\n";
			close(fd);
			exit(0);
		}
	}

	dataTemp  = (float*) malloc (nF * NUM_STREAMS * sizeof(float));
	if (!dataTemp){
		cerr << "Can't malloc data area.\n";
		close(fd);
		exit(0);
	}

	for (unsigned int i=0; i<numSpectra; i++){
		if (i==start){
			printSpectraHeader();
		}
		if (i!=0){ // skip reading the first header, since we already read it
			readHeader();
			if ( (hdrTmp.nFreqs != nF) || (hdrTmp.nInts != nI)){
				cerr << "Error, geometry changed mid file.\n";
				free(data[0]);free(data[1]);free(data[2]);free(data[3]);free(dataTemp);
				close(fd);
				exit(0);
			}
		}
		//cout<<"llllllllllllllllllllllllllllllllllllll " << dec<< i << endl; fflush(stdout);


		readData();
		for(int str=0; str<NUM_STREAMS; str++){
			satCount[str][i] = (double)hdrTmp.satCount[str];
			fills[str][i] = ((double)hdrTmp.fills[str])/((double)nI);
			memcpy((void*)&data[str][i*nF], (void*)&dataTemp[str*nF], nF*sizeof(float));
		}
	}
	free(dataTemp);
	for (int i=0; i<NUM_STREAMS; i++){
		timeSeriesTotalPower[i]=(double*) malloc(numSpectra * sizeof(double));
		integratedSpectra[i]=(double*) malloc(nF * sizeof(double));
		bzero(timeSeriesTotalPower[i],numSpectra*sizeof(double));
		bzero(integratedSpectra[i],nF*sizeof(double));
		for(int j=0;j<(int)numSpectra;j++){
			if ((j>=(int)realStart)&&(j<(int)(realStart+realLength))){
				for(int f=0; f<(int)nF; f++){
					timeSeriesTotalPower[i][j] += ((double)(data[i][j*nF+f]))/((double)nF);
					integratedSpectra[i][f]    += ((double)(data[i][j*nF+f]))/((double)realLength);
				}
			}
		}
	}
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


	readSpectra();
	cout << "Found " <<dec<< numSpectra << " spectra of size "<< nF << " frequency channels (integration count = "<<nI<<" )." << endl;fflush(stdout);
	uint64_t realStart = (start<numSpectra) ? start : numSpectra;
	uint64_t realLength = ((realStart+length) <numSpectra) ? length : (numSpectra - realStart);
	if (!realLength){
		cout << "Supplied start and length are outside of detected spectra. Exitting...\n";
		return -1;
	}
	//printSpectra(tunpolidx,realStart,realLength);

	try
    {
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "Plotting spectrograms....\n";
		Gnuplot plotter0(true);
		//plotter0.cmd("set palette rgbformulae 30,32,7");
		plotter0.cmd("set multiplot;");
		plotter0.cmd("set size 0.5,0.5;");

		for (int i=0;i<NUM_STREAMS;i++){
			int iTun=(i&0x2)>>1;
			int iPol=(i&0x1);

			stringstream s_org;
			s_org << "set origin " << (((double)iPol)*0.5) << "," << (((double)(1-iTun))*0.5) << ";";
			plotter0.cmd(s_org.str());
			double center = (((i & 2)>>1) ? freq2 : freq1)/1e6;
			double frange = (2*bandwidth / ((double)nF))/1e6;
			double xmin   = center - frange * (nF/2);
			double xmax   = center + frange * (nF/2-1);
			plotter0.set_xrange(xmin,xmax);


			double timestep = ((double)(nF*nI))/bandwidth;
			double ymin     = ((double)(realStart)) * timestep;
			double ymax     = ((double)(realStart+realLength-1)) * timestep;
			plotter0.set_yrange(ymin,ymax);



			stringstream s_tit;
			s_tit.clear();
			s_tit << "Waterfall (" << realStart << "::" << (realStart+realLength) << ") for " << (((i&0x2)>>1)+1) << ((i&0x1)?"Y":"X");
			plotter0.set_title(s_tit.str());
			if (iTun==1)
				plotter0.set_xlabel("Frequency (MHz)");
			else
				plotter0.set_xlabel("");

			if (iPol==0)
				plotter0.set_ylabel("Time (s)");
			else
				plotter0.set_ylabel("");

			if (iPol==1)
				plotter0.cmd("set cblabel \"Power (dB)\"");
			else
				plotter0.cmd("set cblabel \"\"");

			plotter0.plot_imagef(&data[i][realStart*nF],xmin,xmax,nF,ymin,ymax,realLength,"");
		}
		plotter0.cmd("unset multiplot");
		//plotter0.showonscreen();

		//timeSeriesTotalPower[i][j-start] += data[i][j*nF+f];
		//integratedSpectra[i][f]    += data[i][j*nF+f];

		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "Plotting integrated spectrograms....\n";
		cout << "Tuning 1 center frequency: " << ((uint64_t)freq1) << dec << " MHz.\n";
		cout << "Tuning 2 center frequency: " << ((uint64_t)freq2) << dec << " MHz.\n";
		Gnuplot plotter1(true);
		plotter1.cmd("set multiplot;");
		plotter1.cmd("set size 0.5,0.5;");
		for (int i=0;i<NUM_STREAMS;i++){
			int iTun=(i&0x2)>>1;
			int iPol=(i&0x1);
			stringstream s_org;
			s_org << "set origin " << (((double)iPol)*0.5) << "," << (((double)(1-iTun))*0.5) << ";";
			plotter1.cmd(s_org.str());
			double center = (((i & 2)>>1) ? freq2 : freq1)/1e6;
			double frange = (2*bandwidth / ((double)nF))/1e6;
			double xmin   = center - frange * (nF/2);
			double xmax   = center + frange * (nF/2-1);
			double xstep  = (xmax-xmin)/((double)nF);
			vector<float> x,y;
			for(int f=0; f<(int)nF; f++){
				x.push_back(((double)f)*xstep + xmin);
				y.push_back(integratedSpectra[i][f]);
			}
			plotter1.set_xrange(xmin,xmax);

			stringstream s_tit;
			s_tit << "Integrated spectra (" << realStart << "::" << (realStart+realLength) << ") for " << (((i&0x2)>>1)+1) << ((i&0x1)?"Y":"X");
			plotter1.set_title(s_tit.str());
			if (iTun==1)
				plotter1.set_xlabel("Frequency (MHz)");
			else
				plotter1.set_xlabel("");

			if (iPol==0)
				plotter1.set_ylabel("Power (dB)");
			else
				plotter1.set_ylabel("");
			plotter1.set_style("linespoints");

			plotter1.plot_xy(x,y,"");
		}
		plotter1.cmd("unset multiplot");


		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "Plotting time-series total power....\n";
		Gnuplot plotter2(true);
		plotter2.cmd("set multiplot;");
		plotter2.cmd("set size 0.5,0.5;");
		for (int i=0;i<NUM_STREAMS;i++){
			int iTun=(i&0x2)>>1;
			int iPol=(i&0x1);
			stringstream s_org;
			s_org << "set origin " << (((double)iPol)*0.5) << "," << (((double)(1-iTun))*0.5) << ";";
			plotter2.cmd(s_org.str());

			double xstep = ((double)(nF*nI))/bandwidth;
			double xmin  = ((double)(realStart)) * xstep;
			double xmax  = ((double)(realStart+realLength-1)) * xstep;


			vector<float> x,y;
			for(int j=0; j<(int)realLength; j++){
				x.push_back(((double)j)*xstep + xmin);
				y.push_back(timeSeriesTotalPower[i][j+realStart]);
			}
			plotter2.set_xrange(xmin,xmax);
			stringstream s_tit;
			s_tit << "Total power (" << realStart << "::" << (realStart+realLength) << ") for " << (((i&0x2)>>1)+1) << ((i&0x1)?"Y":"X");
			plotter2.set_title(s_tit.str());
			if (iTun==1)
				plotter2.set_xlabel("Time (s)");
			else
				plotter2.set_xlabel("");

			if (iPol==0)
				plotter2.set_ylabel("Power (dB)");
			else
				plotter2.set_ylabel("");
			plotter2.set_style("linespoints");

			plotter2.plot_xy(x,y,"");
		}
		plotter2.cmd("unset multiplot");


		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "Plotting time-series fills....\n";
		Gnuplot plotter3(true);
		plotter3.cmd("set multiplot;");
		plotter3.cmd("set size 0.5,0.5;");
		for (int i=0;i<NUM_STREAMS;i++){
			int iTun=(i&0x2)>>1;
			int iPol=(i&0x1);
			stringstream s_org;
			s_org << "set origin " << (((double)iPol)*0.5) << "," << (((double)(1-iTun))*0.5) << ";";
			plotter3.cmd(s_org.str());

			double xstep = ((double)(nF*nI))/bandwidth;
			double xmin  = ((double)(realStart)) * xstep;
			double xmax  = ((double)(realStart+realLength-1)) * xstep;


			vector<float> x,y;
			for(int j=0; j<(int)realLength; j++){
				x.push_back(((double)j)*xstep + xmin);
				y.push_back(fills[i][j+realStart]);
			}
			plotter3.set_xrange(xmin,xmax);
			stringstream s_tit;
			s_tit << "Fills " << realStart << " through " << (realStart+realLength) << " for " << (((i&0x2)>>1)+1) << ((i&0x1)?"Y":"X");
			plotter3.set_title(s_tit.str());
			if (iTun==1)
				plotter3.set_xlabel("Time (s)");
			else
				plotter3.set_xlabel("");

			if (iPol==0)
				plotter3.set_ylabel("Fill fraction");
			else
				plotter3.set_ylabel("");
			plotter3.set_style("lines");

			plotter3.plot_xy(x,y,"");
		}
		plotter3.cmd("unset multiplot");


		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "********************************************************************\n";
		cout << "Plotting saturation counts....\n";
		Gnuplot plotter4(true);
		plotter4.cmd("set multiplot;");
		plotter4.cmd("set size 0.5,0.5;");
		for (int i=0;i<NUM_STREAMS;i++){
			int iTun=(i&0x2)>>1;
			int iPol=(i&0x1);
			stringstream s_org;
			s_org << "set origin " << (((double)iPol)*0.5) << "," << (((double)(1-iTun))*0.5) << ";";
			plotter4.cmd(s_org.str());

			double xstep = ((double)(nF*nI))/bandwidth;
			double xmin  = ((double)(realStart)) * xstep;
			double xmax  = ((double)(realStart+realLength-1)) * xstep;


			vector<float> x,y;
			for(int j=0; j<(int)realLength; j++){
				x.push_back(((double)j)*xstep + xmin);
				y.push_back(satCount[i][j+realStart]);
			}
			plotter4.set_xrange(xmin,xmax);
			stringstream s_tit;
			s_tit << "Saturation counts " << realStart << " through " << (realStart+realLength) << " for " << (((i&0x2)>>1)+1) << ((i&0x1)?"Y":"X");
			plotter4.set_title(s_tit.str());
			if (iTun==1)
				plotter4.set_xlabel("Time (s)");
			else
				plotter4.set_xlabel("");

			if (iPol==0)
				plotter4.set_ylabel("Counts");
			else
				plotter4.set_ylabel("");
			plotter4.set_style("lines");

			plotter4.plot_xy(x,y,"");
		}
		plotter4.cmd("unset multiplot");


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
