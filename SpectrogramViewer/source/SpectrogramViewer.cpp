// Example for C++ Interface to Gnuplot

// requirements:
// * gnuplot has to be installed (http://www.gnuplot.info/download.html)
// * for Windows: set Path-Variable for Gnuplot path (e.g. C:/program files/gnuplot/bin)
//             or set Gnuplot path with: Gnuplot::set_GNUPlotPath(const std::string &path);


#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdint.h>
#include <cassert>

#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "gnuplot_i.hpp" //Gnuplot class handles POSIX-Pipe-communikation with Gnuplot
#include <math.h>
#include <limits>

void wait_for_key(); // Programm halts until keypress
const double MHz=1e6;
using namespace std;

#define NUM_TUNINGS 2
#define NUM_POLS    2
#define NUM_STREAMS (NUM_TUNINGS * NUM_POLS)
#define DP_BASE_FREQ_HZ         196e6
#define DRX_SAMPLES_PER_FRAME   4096
#define FREQ_CODE_FACTOR        (DP_BASE_FREQ_HZ / (double)0x100000000)

enum Pol{POL_X=0,POL_Y=1};

enum PolType{ X=0x01, Y=0x02, XandY=0x03};
#define PolSize(p) (__builtin_popcount(p))

enum SamplePolOrganization{ Interleaved, Noninterleaved};

enum StokesProduct {
	INVALID_STOKES = 0x00,
	XX   = 0x01, XY       = 0x02, YX = 0x04, YY   = 0x08,
	I    = 0x10, Q        = 0x20, U  = 0x40, V    = 0x80,
	XXYY = 0x09, XXXYYXYY = 0x0F, IV = 0x90, IQUV = 0xF0
}; // thanks Jake :)
#define StokesSize(s) (__builtin_popcount(s))
string stokesName(StokesProduct s);
StokesProduct nameToStokes(string name);
#define StokesSupported(s)\
	((s==XXYY)||\
	 (s==IQUV)||\
	 (s==IV)\
	 /* more here as supported */\
	)
#define StokesTypeLegalOnInputType(s,p)\
	(\
		(s==XX && (p!=Y)) ||\
		(s==YY && (p!=X)) ||\
		(p==XandY)\
	)
enum CorrProduct{
	cINVALID = 0x00,
	cXY      = 0x02
};
#define CorrSize(c) (__builtin_popcount(c))
string corrName(CorrProduct c);
CorrProduct   nameToCorr(string name);
#define CorrSupported(c)\
	((c==cXY)\
	 /* more here as supported */\
	)
#define CorrTypeLegalOnInputType(c,p)\
	(\
		((c==cXY)&&(p==XandY))\
	)
typedef struct __ProductType{
	__ProductType():stokes(INVALID_STOKES),isCorr(false){};
	__ProductType(StokesProduct s):stokes(s),isCorr(false){};
	__ProductType(CorrProduct c):corr(c),isCorr(true){};
	StokesProduct toStokes(){             return isCorr ? INVALID_STOKES : stokes;}
	CorrProduct   toCorr(){               return isCorr ? corr           : cINVALID;}
	bool legalForInputType(PolType pin){  return isCorr ? (CorrTypeLegalOnInputType(corr, pin)) : (StokesTypeLegalOnInputType(stokes,pin));}
	bool isSupported(){                   return isCorr ? (CorrSupported(corr))                 : (StokesSupported(stokes));}
	size_t numberOutputProducts(){        return isCorr ? CorrSize(corr)                        : StokesSize(stokes);}
	bool   outputProductsComplex(){       return isCorr;}
	string name(){                        return isCorr ? corrName(corr) : stokesName(stokes);}
	union{
		StokesProduct stokes;
		CorrProduct corr;
		uint8_t byte;
	};
	bool isCorr;
}ProductType;
#define ProductSize(p) (__builtin_popcount(p.byte))




string stokesName(StokesProduct s){

	switch(s){
		case XX:       return "XX";
		case XY:       return "XY";
		case YX:       return "YX";
		case YY:       return "YY";
		case XXYY:     return "XXYY";
		case XXXYYXYY: return "XXXYYXYY";
		case I:        return "I";
		case Q:        return "Q";
		case U:        return "U";
		case V:        return "V";
		case IV:       return "IV";
		case IQUV:     return "IQUV";
		default:	   return "Unknown stokes mode";
	}
}
string corrName(CorrProduct c){
	switch(c){
		case cXY: return "XY";
		default:  return "Unknown correlator mode";
	}
}

StokesProduct toStokes(int x){
	switch(x){
		case 1:        return XX;
		case 2:        return XY;
		case 4:        return YX;
		case 8:        return YY;
		case 9:        return XXYY;
		case 15:       return XXXYYXYY;
		case 16:       return I;
		case 32:       return Q;
		case 64:       return U;
		case 128:      return V;
		case 144:      return IV;
		case 240:      return IQUV;
		default:	   return INVALID_STOKES;
	}
}
CorrProduct toCorr(int x){
	switch(x){
		case 2:        return cXY;
		default:	   return cINVALID;
	}
}

typedef struct __DrxSpectraHeader{
	uint32_t			MAGIC1;     // must always equal 0xC0DEC0DE
	uint64_t 			timeTag0;
	uint16_t 			timeOffset;
	uint16_t 			decFactor;
	uint32_t 			freqCode[2];
	uint32_t			fills[4];
	uint8_t				errors[4];
	uint8_t 			beam;
	union{
		uint8_t         stokes_format;
		uint8_t         xcp_format;
	};
	uint8_t             spec_version;
	union{
		struct {
			uint8_t     flag_xcp:1;
			uint8_t     flag_reserved:5;
			uint8_t     flag_k:2; // kurtosis
		};
		uint8_t         reserved;
	};
	union{
		uint32_t 			nFreqs;
		uint32_t 			nPerFrame;
	};
	uint32_t 			nInts;
	uint32_t 			satCount[4];
	uint32_t			MAGIC2;     // must always equal 0xED0CED0C
} __attribute__((packed)) DrxSpectraHeader;

#define COUNT 128

/*
float* data[NUM_TUNINGS];
float* dataTemp;
double* fills[NUM_TUNINGS];
int fd;
uint32_t nF;
uint32_t nI;
uint32_t nProd;
DrxSpectraHeader hdr;
DrxSpectraHeader hdrTmp;
uint64_t numSpectra;
uint64_t start;
uint64_t length;
double** timeSeriesTotalPower;
double** integratedSpectra;
double**  satCount;

double freq1;
double freq2;
double bandwidth;
*/

void* MALLOC(size_t sz){
	void* rv = malloc(sz);
	if (!rv){
		cout << "Allocation failure\n";
		exit(-1);
	}
	bzero(rv, sz);
	return rv;
}
class SpectraBuffer{
public:
	static double abs(double x){
		if (x>=0.0)
			return x;
		else
			return (-1.0*x);
	}
	SpectraBuffer(size_t _numSpectra, size_t _numTunings, size_t _numFreqs, size_t _numProducts, size_t _numInts, bool _logScale):
		logScale(_logScale),
		headers(NULL),
		fills(NULL),
		satCounts(NULL),
		errors(NULL),
		data(NULL),
		integrated(NULL),
		power(NULL),
		timeTagTime(NULL),
		numSpectra(_numSpectra),
		numFreqs(_numFreqs),
		numInts(_numInts),
		numTunings(_numTunings),
		numProducts(_numProducts),
		initialTimeTag(0)

	{
		headers     = (DrxSpectraHeader*) MALLOC(numSpectra * sizeof(DrxSpectraHeader));
		data        = (float*)            MALLOC(numSpectra * numFreqs * numTunings * numProducts*sizeof(float));
		fills       = (double**)          MALLOC(NUM_STREAMS * sizeof(double*));
		satCounts   = (double**)          MALLOC(NUM_STREAMS * sizeof(double*));
		errors      = (float**)           MALLOC(NUM_STREAMS * sizeof(float*));
		integrated  = (double***)         MALLOC(_numTunings * sizeof(double**));
		power       = (double***)         MALLOC(_numTunings * sizeof(double**));
		timeTagTime = (double*)           MALLOC(numSpectra  * sizeof(double));

		for (size_t s=0; s<NUM_STREAMS; s++){
			fills[s]     = allocTimeSeries<double>();
			satCounts[s] = allocTimeSeries<double>();
			errors[s]    = allocTimeSeries<float>();
		}
		for (size_t t=0; t<numTunings; t++){
			power[t]      = (double**) MALLOC(numProducts * sizeof(double*));
			integrated[t] = (double**) MALLOC(numProducts * sizeof(double*));
			for (size_t p=0; p<numProducts; p++){
				integrated[t][p] = allocFreqSeries<double>();
				power[t][p]      = allocTimeSeries<double>();
			}
		}
	}
	virtual ~SpectraBuffer(){
		if (data)        free(data);
		if (headers)     free(headers);
		for (size_t s=0; s<NUM_STREAMS; s++){
			if (fills[s])     free(fills[s]);
			if (satCounts[s]) free(satCounts[s]);
			if (errors[s])    free(errors[s]);
		}
		for (size_t t=0; t<numTunings; t++){
			if (integrated[t]){
				for (size_t p=0; p<numProducts; p++){
					if (integrated[t][p])
						free(integrated[t][p]);
				}
				free (integrated[t]);
			}
			if (power[t]){
				for (size_t p=0; p<numProducts; p++){
					if (power[t][p])
						free(power[t][p]);
				}
				free (power[t]);
			}
		}
		if (timeTagTime) free(timeTagTime);
	}

	inline float* where(size_t whichSpectra, size_t whichTuning, size_t whichFreq, size_t whichProduct){
		size_t linindex =
/*				(whichSpectra * (numTunings * numFreqsOrSpf * numProducts)) +
				(whichTuning *  (             numFreqsOrSpf * numProducts)) +
				(whichFreq *    (                        numProducts)) +
				(whichProduct * (                                  1));
*/
		(whichTuning  * numFreqs * numSpectra* numProducts)+
		(whichProduct * numFreqs * numSpectra)+
		(whichSpectra * numFreqs)+
		(whichFreq);
		return &data[linindex];
	}
	inline bool checkCompatible(DrxSpectraHeader*hdr){
		return
			(
					(hdr->MAGIC1 == 0xC0DEC0DE) &&
					(hdr->MAGIC2 == 0xED0CED0C) &&
					(StokesSize(toStokes(hdr->stokes_format)) == numProducts) &&
					(hdr->nFreqs == numFreqs)
			);
	}
	inline void unpack(size_t whichSpectra, DrxSpectraHeader*hdr, float*fdata, bool findZeros){
		assert(whichSpectra < numSpectra);
		if (!whichSpectra)
			initialTimeTag = hdr->timeTag0;
		timeTagTime[whichSpectra] = ((double)(hdr->timeTag0 - initialTimeTag))/DP_BASE_FREQ_HZ;
		memcpy((void*)&headers[whichSpectra],(void*) hdr, sizeof(DrxSpectraHeader));
		for (int s=0; s<NUM_STREAMS; s++){
			fills[s][whichSpectra]     = ((double)hdr->fills[s])/((double)numInts);
			satCounts[s][whichSpectra] = (double)hdr->satCount[s]/((double)numInts);
			errors[s][whichSpectra]    = (hdr->errors[s]!=0) ? 1.0 : 0.0;
		}
		size_t i=0;
		for (size_t t=0; t<numTunings; t++){
			for (size_t f=0; f<numFreqs; f++){
				for (size_t p=0; p<numProducts; p++){
					double sample = (double) fdata[i];
					if (findZeros){
						bool z = (sample==0);
						if (z){
							cout << "ZEROFIND: Frame# " << std::dec << whichSpectra << " Tuning# "<< std::dec <<t<<" Channel# " << std::dec << f  << " Product# " << std::dec << p << ":::: [Real-part]" << std::endl;
						}
					}
					if (logScale){
						if (isnan(fdata[i])){
							cout << "NaN in input data.\n";
						}
						sample = abs(sample);
						if (sample !=  0.0)
							sample = 10.0 * log10(sample);
						if (isnan(sample)){
							cout << "NaN in output data.\n";
						}
					}


					*where(whichSpectra,t,f,p) =  sample;
					*intPtr(t,p,f)             += sample/((double)numSpectra);
					*powPtr(t,p,whichSpectra)  += sample/((double)numFreqs);
					i++;
				}
			}
		}
	}
	double getMaxTime(){
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numSpectra; ws++){
			if (timeTagTime[ws]>rv)
				rv=timeTagTime[ws];
		}
		return rv;
	}
	double getMinTime(){
		double rv = (numeric_limits<double>::max());
		for (size_t ws=0; ws<numSpectra; ws++){
			if (timeTagTime[ws]<rv)
				rv=timeTagTime[ws];
		}
		return rv;
	}
	double getMinFill(size_t whichStream){
		assert(whichStream < 4);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numSpectra; ws++){
			double fill=*fillPtr(ws, whichStream);
			if (fill<rv)
				rv=fill;
		}
		return rv;
	}
	double getMaxFill(size_t whichStream){
		assert(whichStream < 4);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numSpectra; ws++){
			double fill=*fillPtr(ws, whichStream);
			if (fill>rv)
				rv=fill;
		}
		return rv;
	}
	double getMinSat (size_t whichStream){
		assert(whichStream < 4);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numSpectra; ws++){
			double sat=*satPtr(ws, whichStream);
			if (sat<rv)
				rv=sat;
		}
		return rv;
	}
	double getMaxSat (size_t whichStream){
		assert(whichStream < 4);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numSpectra; ws++){
			double sat=*satPtr(ws, whichStream);
			if (sat>rv)
				rv=sat;
		}
		return rv;
	}
	double getMinPwr (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numSpectra; ws++){
			double pwr=*powPtr(whichTuning, whichProduct, ws);
			if (pwr<rv)
				rv=pwr;
		}
		return rv;
	}
	double getMaxPwr (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numSpectra; ws++){
			double pwr=*powPtr(whichTuning, whichProduct, ws);
			if (pwr>rv)
				rv=pwr;
		}
		return rv;
	}
	double getMinInt (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = numeric_limits<double>::max();
		for (size_t wf=0; wf<numFreqs; wf++){
			double int_=*intPtr(whichTuning, whichProduct, wf);
			if (int_<rv)
				rv=int_;
		}
		return rv;
	}
	double getMaxInt (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = (-numeric_limits<double>::max());
		for (size_t wf=0; wf<numFreqs; wf++){
			double int_=*intPtr(whichTuning, whichProduct, wf);
			if (int_>rv)
				rv=int_;
		}
		return rv;
	}

	inline double* timePtr(){
		return timeTagTime;
	}
	inline double* fillPtr(size_t whichSpectra, size_t whichStream){
		assert(whichStream < 4);
		assert(whichSpectra < numSpectra);
		return &fills[whichStream][whichSpectra];
	}
	inline double* satPtr(size_t whichSpectra, size_t whichStream){
		assert(whichStream < 4);
		assert(whichSpectra < numSpectra);
		return &satCounts[whichStream][whichSpectra];
	}
	inline float* errPtr(size_t whichSpectra, size_t whichStream){
		assert(whichStream < 4);
		assert(whichSpectra < numSpectra);
		return &errors[whichStream][whichSpectra];
	}
	inline double* intPtr(size_t whichTuning, size_t whichProduct, size_t whichFreq){
		assert(whichFreq < numFreqs);
		assert(whichProduct < numProducts);
		assert(whichTuning < numTunings);
		return &integrated[whichTuning][whichProduct][whichFreq];
	}
	inline double* powPtr(size_t whichTuning, size_t whichProduct, size_t whichSpectra){
		assert(whichSpectra < numSpectra);
		assert(whichProduct < numProducts);
		assert(whichTuning < numTunings);
		return &power[whichTuning][whichProduct][whichSpectra];
	}

	template <class T> inline T* allocTimeSeries(){
		T* rv = (T*) malloc(numSpectra * sizeof(T));
		if (!rv){
			cout << "Allocation failure\n";
			exit(-1);
		}
		bzero((void*)rv, numSpectra * sizeof(T));
		return rv;
	}
	template <class T> inline T* allocFreqSeries(){
		T* rv = (T*) malloc(numFreqs * sizeof(T));
		if (!rv){
			cout << "Allocation failure\n";
			exit(-1);
		}
		bzero((void*)rv, numFreqs * sizeof(T));
		return rv;
	}

	void printSpectra(size_t whichSpectra, size_t whichTuning, size_t whichProduct){
		assert(whichSpectra < numSpectra);
		assert(whichProduct < numProducts);
		assert(whichTuning < 4);
		for (size_t f=0; f<numFreqs; f++){
			cout << *where(whichSpectra, whichTuning, f, whichProduct) << " ";
		}
		cout << endl;
	}
	void printSpectraHeader(DrxSpectraHeader* hdr){
		cout << "=============================================\n";
		cout << "MAGIC1         " << hex << hdr->MAGIC1            << endl;
		cout << "VERSION        " << dec << (int)hdr->spec_version << endl;
		cout << "stokes         " << stokesName(toStokes(hdr->stokes_format))    << endl;
		cout << "timeTag0       " << dec << hdr->timeTag0          << endl;
		cout << "timeOffset     " << dec << hdr->timeOffset        << endl;
		cout << "decFactor      " << dec << hdr->decFactor         << endl;
		cout << "freqCode[1]    " << dec << hdr->freqCode[0]       << endl;
		cout << "freqCode[2]    " << dec << hdr->freqCode[1]       << endl;
		cout << "fill[0,X]      " << dec << hdr->fills[0]          << endl;
		cout << "fill[0,Y]      " << dec << hdr->fills[1]          << endl;
		cout << "fill[1,X]      " << dec << hdr->fills[2]          << endl;
		cout << "fill[1,Y]      " << dec << hdr->fills[3]          << endl;
		cout << "sat count[0,X] " << dec << hdr->satCount[0]       << endl;
		cout << "sat count[0,Y] " << dec << hdr->satCount[1]       << endl;
		cout << "sat count[1,X] " << dec << hdr->satCount[2]       << endl;
		cout << "sat count[1,Y] " << dec << hdr->satCount[3]       << endl;
		cout << "error[0,X]     " << dec << (int) hdr->errors[0]   << endl;
		cout << "error[0,Y]     " << dec << (int) hdr->errors[1]   << endl;
		cout << "error[1,X]     " << dec << (int) hdr->errors[2]   << endl;
		cout << "error[1,Y]     " << dec << (int) hdr->errors[3]   << endl;
		cout << "beam           " << hex << (int) hdr->beam        << endl;
		cout << "nFreqs         " << dec << hdr->nFreqs            << endl;
		cout << "nInts          " << dec << hdr->nInts             << endl;
		cout << "MAGIC2         " << hex << hdr->MAGIC2            << endl;
		cout << "=============================================\n";
	}




private:
	bool      logScale;
	DrxSpectraHeader* headers;
	double**  fills;
	double**  satCounts;
	float**   errors;
	float*    data;
	double*** integrated; //power int across time
	double*** power;      //power int across freq
	double*   timeTagTime;
	size_t    numSpectra;
	size_t    numFreqs;
	size_t    numInts;
	size_t    numTunings;
	size_t    numProducts;
	uint64_t  initialTimeTag;
};
class CorrBuffer{
public:
	static double abs(double x){
		if (x>=0.0)
			return x;
		else
			return (-1.0*x);
	}
	CorrBuffer(size_t _numFrames, size_t _numTunings, size_t _numSamplesPerFrame, size_t _numProducts, size_t _numInts, bool _logScale):
		logScale(_logScale),
		headers(NULL),
		fills(NULL),
		satCounts(NULL),
		errors(NULL),
		data(NULL),
		power(NULL),
		angle(NULL),
		timeTagTime(NULL),
		fullTime(NULL),
		numFrames(_numFrames),
		numSamplesPerFrame(_numSamplesPerFrame),
		numInts(_numInts),
		numTunings(_numTunings),
		numProducts(_numProducts),
		initialTimeTag(0)

	{
		headers     = (DrxSpectraHeader*) MALLOC(numFrames * sizeof(DrxSpectraHeader));
		data        = (double*)           MALLOC(numFrames * numSamplesPerFrame * numTunings * numProducts*sizeof(double)*2);
		fills       = (double**)          MALLOC(NUM_STREAMS * sizeof(double*));
		satCounts   = (double**)          MALLOC(NUM_STREAMS * sizeof(double*));
		errors      = (float**)           MALLOC(NUM_STREAMS * sizeof(float*));
		power       = (double***)         MALLOC(_numTunings * sizeof(double**));
		angle       = (double***)         MALLOC(_numTunings * sizeof(double**));
		timeTagTime = (double*)           MALLOC(numFrames * sizeof(double));
		fullTime    = (double*)           MALLOC(numFrames * numSamplesPerFrame * sizeof(double));

		for (size_t s=0; s<NUM_STREAMS; s++){
			fills[s]     = allocTimeSeries<double>();
			satCounts[s] = allocTimeSeries<double>();
			errors[s]    = allocTimeSeries<float>();
		}
		for (size_t t=0; t<numTunings; t++){
			power[t]      = (double**) MALLOC(numProducts * sizeof(double*));
			angle[t]      = (double**) MALLOC(numProducts * sizeof(double*));
			for (size_t p=0; p<numProducts; p++){
				power[t][p]      = (double*) MALLOC(numFrames * numSamplesPerFrame * sizeof(double));
				angle[t][p]      = (double*) MALLOC(numFrames * numSamplesPerFrame * sizeof(double));
			}
		}
	}
	virtual ~CorrBuffer(){
		if (data)        free(data);
		if (headers)     free(headers);
		for (size_t s=0; s<NUM_STREAMS; s++){
			if (fills[s])     free(fills[s]);
			if (satCounts[s]) free(satCounts[s]);
			if (errors[s])    free(errors[s]);
		}
		for (size_t t=0; t<numTunings; t++){
			if (power[t]){
				for (size_t p=0; p<numProducts; p++){
					if (power[t][p])
						free(power[t][p]);
				}
				free (power[t]);
			}
			if (angle[t]){
				for (size_t p=0; p<numProducts; p++){
					if (angle[t][p])
						free(angle[t][p]);
				}
				free (angle[t]);
			}
		}
		if (timeTagTime) free(timeTagTime);
		if (fullTime)    free(fullTime);
	}

	inline double* where(size_t whichFrame, size_t whichTuning, size_t whichSample, size_t whichProduct, bool realPart){
		size_t linindex =
			(realPart ? 0 : numTunings * numSamplesPerFrame * numFrames* numProducts) +
			(whichTuning  * numSamplesPerFrame * numFrames* numProducts)+
			(whichProduct * numSamplesPerFrame * numFrames)+
			(whichFrame * numSamplesPerFrame)+
			(whichSample);
		return &data[linindex];
	}
	inline bool checkCompatible(DrxSpectraHeader*hdr){
		return
			(
					(hdr->MAGIC1 == 0xC0DEC0DE) &&
					(hdr->MAGIC2 == 0xED0CED0C) &&
					(CorrSize(toCorr(hdr->xcp_format)) == numProducts) &&
					(hdr->nPerFrame == numSamplesPerFrame)
			);
	}
	inline void unpack(size_t whichFrame, DrxSpectraHeader*hdr, float*fdata, bool findZeros){
		assert(whichFrame < numFrames);

		if (!whichFrame)
			initialTimeTag = hdr->timeTag0;
		timeTagTime[whichFrame] = ((double)(hdr->timeTag0 - initialTimeTag))/DP_BASE_FREQ_HZ;
		for (size_t s=0; s<numSamplesPerFrame; s++){
			fullTime[(whichFrame*numSamplesPerFrame) + s] = ((double)(hdr->timeTag0 - initialTimeTag + (numInts * hdr->decFactor * s)))/DP_BASE_FREQ_HZ;
		}

		memcpy((void*)&headers[whichFrame],(void*) hdr, sizeof(DrxSpectraHeader));
		for (int s=0; s<NUM_STREAMS; s++){
			fills[s][whichFrame]     = ((double)hdr->fills[s])/((double)numInts);
			satCounts[s][whichFrame] = (double)hdr->satCount[s]/((double)numInts);
			errors[s][whichFrame]    = (hdr->errors[s]!=0) ? 1.0 : 0.0;
		}
		size_t i=0;
		for (size_t t=0; t<numTunings; t++){
			cout << "###################################################################" << endl;
			printSpectra(whichFrame, t, 0);
			cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
			for (size_t s=0; s<numSamplesPerFrame; s++){
				for (size_t p=0; p<numProducts; p++){
					double re = (double) fdata[i];
					double im = (double) fdata[i+1];
					if (findZeros){
						bool rez = (re==0);
						bool imz = (im==0);
						if (rez || imz){
							cout << "ZEROFIND: Frame# " << std::dec << whichFrame << " Tuning# "<< std::dec <<t<<" Sample# " << std::dec << s  << " Product# " << std::dec << p << "::::" << (rez ? " [Real-part] ": "") << (imz ? " [Imaginary-part] ": "") << std::endl;
						}
					}
					double pow = sqrt(re*re+im*im);
					double angle = atan2(im,re);
					if (logScale){
						if (isnan(pow)){
							cout << "NaN in input data.\n";
						}
						pow = abs(pow);
						if (pow !=  0.0)
							pow = 10.0 * log10(pow);
						if (isnan(pow)){
							cout << "NaN in output data.\n";
						}
					}
					*where(whichFrame,t,s,p,true)  = re;
					*where(whichFrame,t,s,p,false) = im;
					*powPtr(t,p,whichFrame,s)      = pow;
					*anglePtr(t,p,whichFrame,s)    = angle;
					i += 2;
				}
			}
		}
	}
	double getMaxTime(){
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			if (timeTagTime[ws]>rv)
				rv=timeTagTime[ws];
		}
		return rv;
	}
	double getMinTime(){
		double rv = (numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			if (timeTagTime[ws]<rv)
				rv=timeTagTime[ws];
		}
		return rv;
	}
	double getMaxFullTime(){
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<(numFrames*numSamplesPerFrame); ws++){
			if (fullTime[ws]>rv)
				rv=fullTime[ws];
		}
		return rv;
	}
	double getMinFullTime(){
		double rv = (numeric_limits<double>::max());
		for (size_t ws=0; ws<(numFrames*numSamplesPerFrame); ws++){
			if (fullTime[ws]<rv)
				rv=fullTime[ws];
		}
		return rv;
	}

	double getMinFill(size_t whichStream){
		assert(whichStream < 4);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numFrames; ws++){
			double fill=*fillPtr(ws, whichStream);
			if (fill<rv)
				rv=fill;
		}
		return rv;
	}
	double getMaxFill(size_t whichStream){
		assert(whichStream < 4);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			double fill=*fillPtr(ws, whichStream);
			if (fill>rv)
				rv=fill;
		}
		return rv;
	}
	double getMinSat (size_t whichStream){
		assert(whichStream < 4);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numFrames; ws++){
			double sat=*satPtr(ws, whichStream);
			if (sat<rv)
				rv=sat;
		}
		return rv;
	}
	double getMaxSat (size_t whichStream){
		assert(whichStream < 4);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			double sat=*satPtr(ws, whichStream);
			if (sat>rv)
				rv=sat;
		}
		return rv;
	}
	double getMinPwr (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numFrames; ws++){
			for (size_t ws2=0; ws2<numSamplesPerFrame; ws2++){
				double pwr=*powPtr(whichTuning, whichProduct, ws, ws2);
				if (pwr<rv)
					rv=pwr;
			}
		}
		return rv;
	}
	double getMaxPwr (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			for (size_t ws2=0; ws2<numSamplesPerFrame; ws2++){
				double pwr=*powPtr(whichTuning, whichProduct, ws, ws2);
				if (pwr>rv)
					rv=pwr;
			}
		}
		return rv;
	}
	double getMinReal (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numFrames; ws++){
			for (size_t ws2=0; ws2<numSamplesPerFrame; ws2++){
				double re=*where(ws, whichTuning, ws2, whichProduct, true);
				if (re<rv)
					rv=re;
			}
		}
		return rv;
	}
	double getMaxReal (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			for (size_t ws2=0; ws2<numSamplesPerFrame; ws2++){
				double re=*where(ws, whichTuning, ws2, whichProduct, true);
				if (re>rv)
					rv=re;
			}
		}
		return rv;
	}
	double getMinImag (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = numeric_limits<double>::max();
		for (size_t ws=0; ws<numFrames; ws++){
			for (size_t ws2=0; ws2<numSamplesPerFrame; ws2++){
				double im=*where(ws, whichTuning, ws2, whichProduct, false);
				if (im<rv)
					rv=im;
			}
		}
		return rv;
	}
	double getMaxImag (size_t whichTuning, size_t whichProduct){
		assert(whichTuning < numTunings);
		assert(whichProduct < numProducts);
		double rv = (-numeric_limits<double>::max());
		for (size_t ws=0; ws<numFrames; ws++){
			for (size_t ws2=0; ws2<numSamplesPerFrame; ws2++){
				double im=*where(ws, whichTuning, ws2, whichProduct, false);
				if (im>rv)
					rv=im;
			}
		}
		return rv;
	}


	double getMinAngle(){return -M_PIl * 2;}
	double getMaxAngle(){return M_PIl * 2;}

	inline double* timePtr(){
		return timeTagTime;
	}
	inline double* timeFullPtr(){
		return fullTime;
	}
	inline double* fillPtr(size_t whichFrame, size_t whichStream){
		assert(whichStream < 4);
		assert(whichFrame < numFrames);
		return &fills[whichStream][whichFrame];
	}
	inline double* satPtr(size_t whichFrame, size_t whichStream){
		assert(whichStream < 4);
		assert(whichFrame < numFrames);
		return &satCounts[whichStream][whichFrame];
	}
	inline float* errPtr(size_t whichFrame, size_t whichStream){
		assert(whichStream < 4);
		assert(whichFrame < numFrames);
		return &errors[whichStream][whichFrame];
	}

	inline double* powPtr(size_t whichTuning, size_t whichProduct, size_t whichFrame, size_t whichSample){
		assert(whichFrame < numFrames);
		assert(whichProduct < numProducts);
		assert(whichTuning < numTunings);
		assert(whichSample < numSamplesPerFrame);
		return &power[whichTuning][whichProduct][(whichFrame*numSamplesPerFrame)+whichSample];
	}
	inline double* anglePtr(size_t whichTuning, size_t whichProduct, size_t whichFrame, size_t whichSample){
		assert(whichFrame < numFrames);
		assert(whichProduct < numProducts);
		assert(whichTuning < numTunings);
		assert(whichSample < numSamplesPerFrame);
		return &angle[whichTuning][whichProduct][(whichFrame*numSamplesPerFrame)+whichSample];
	}

	template <class T> inline T* allocTimeSeries(){
		T* rv = (T*) malloc(numFrames * sizeof(T));
		if (!rv){
			cout << "Allocation failure\n";
			exit(-1);
		}
		bzero((void*)rv, numFrames * sizeof(T));
		return rv;
	}
	void printSpectra(size_t whichFrame, size_t whichTuning, size_t whichProduct){
		assert(whichFrame < numFrames);
		assert(whichProduct < numProducts);
		assert(whichTuning < 4);
		cout << "---------"<<setw(10) << whichFrame << "---------\n\t";
		for (size_t s=0; s<numSamplesPerFrame; s++){
			cout << *where(whichFrame, whichTuning, s, whichProduct, true)  << " + "
				 << *where(whichFrame, whichTuning, s, whichProduct, false) << "i ";
			if ((s&0x7) == 0x7) cout << "\n\t";
		}
		cout << endl;
	}
	void printSpectraHeader(DrxSpectraHeader* hdr){
		cout << "=============================================\n";
		cout << "MAGIC1         " << hex << hdr->MAGIC1            << endl;
		cout << "VERSION        " << dec << (int)hdr->spec_version << endl;
		cout << "stokes         " << stokesName(toStokes(hdr->stokes_format))    << endl;
		cout << "timeTag0       " << dec << hdr->timeTag0          << endl;
		cout << "timeOffset     " << dec << hdr->timeOffset        << endl;
		cout << "decFactor      " << dec << hdr->decFactor         << endl;
		cout << "freqCode[1]    " << dec << hdr->freqCode[0]       << endl;
		cout << "freqCode[2]    " << dec << hdr->freqCode[1]       << endl;
		cout << "fill[0,X]      " << dec << hdr->fills[0]          << endl;
		cout << "fill[0,Y]      " << dec << hdr->fills[1]          << endl;
		cout << "fill[1,X]      " << dec << hdr->fills[2]          << endl;
		cout << "fill[1,Y]      " << dec << hdr->fills[3]          << endl;
		cout << "sat count[0,X] " << dec << hdr->satCount[0]       << endl;
		cout << "sat count[0,Y] " << dec << hdr->satCount[1]       << endl;
		cout << "sat count[1,X] " << dec << hdr->satCount[2]       << endl;
		cout << "sat count[1,Y] " << dec << hdr->satCount[3]       << endl;
		cout << "error[0,X]     " << dec << (int) hdr->errors[0]   << endl;
		cout << "error[0,Y]     " << dec << (int) hdr->errors[1]   << endl;
		cout << "error[1,X]     " << dec << (int) hdr->errors[2]   << endl;
		cout << "error[1,Y]     " << dec << (int) hdr->errors[3]   << endl;
		cout << "beam           " << hex << (int) hdr->beam        << endl;
		cout << "nFreqs         " << dec << hdr->nFreqs            << endl;
		cout << "nInts          " << dec << hdr->nInts             << endl;
		cout << "MAGIC2         " << hex << hdr->MAGIC2            << endl;
		cout << "=============================================\n";
	}




private:
	bool      logScale;
	DrxSpectraHeader* headers;
	double**  fills;
	double**  satCounts;
	float**   errors;
	double*   data;
	double*** power;
	double*** angle;
	double*   timeTagTime;
	double*   fullTime;
	size_t    numFrames;
	size_t    numSamplesPerFrame;
	size_t    numInts;
	size_t    numTunings;
	size_t    numProducts;
	uint64_t  initialTimeTag;
};

string getProductName(ProductType pt, int p){
	if (pt.isCorr){
		switch(pt.toCorr()){
		case cXY:      if (p==0) return "XY*"; else break;
		default:	   return "Unknown correlator mode";
		}

	}else{
		switch(pt.toStokes()){
			case XX:        if (p==0) return "XX"; else break;
			case XY:        if (p==0) return "XY"; else break;
			case YX:        if (p==0) return "YX"; else break;
			case YY:        if (p==0) return "YY"; else break;
			case XXYY:      if (p==0) return "XX"; else
							if (p==1) return "YY"; else break;
			case XXXYYXYY:	if (p==0) return "XX"; else
							if (p==1) return "XY"; else
							if (p==2) return "YX"; else
							if (p==3) return "YY"; else	break;
			case I:         if (p==0) return "I";  else break;
			case Q:         if (p==0) return "Q";  else break;
			case U:         if (p==0) return "U";  else break;
			case V:         if (p==0) return "V";  else break;
			case IV:        if (p==0) return "I";  else
							if (p==1) return "V";  else break;
			case IQUV:     	if (p==0) return "I"; else
							if (p==1) return "Q"; else
							if (p==2) return "U"; else
							if (p==3) return "V"; else break;
			default:	   return "Unknown stokes mode";
		}
	}
	return "Bad index for this stokes mode";

}


class SpecReader{
public:
	SpecReader(string filename, size_t startSpectra, size_t _count, bool _logScale, bool _findZeros){
		findZeros = _findZeros;
		fd = open(filename.c_str(),  O_RDONLY | O_EXCL , S_IROTH | S_IRUSR |  S_IRGRP | S_IWOTH | S_IWUSR |  S_IWGRP );
		if (fd==-1){
			cout << "Can't open file: '"<<strerror(errno)<<"'.\n";
			exit(-1);
		}
		readHeader();

		logScale		  = _logScale;

		isCorr            = temp_header.flag_xcp == 1;
		type              = isCorr ? ProductType((CorrProduct)temp_header.xcp_format) : ProductType((StokesProduct)temp_header.stokes_format);
		numFreqsOrSpf     = isCorr ? temp_header.nPerFrame : temp_header.nFreqs;
		numInts           = temp_header.nInts;
		numTunings        = 2;
		numProducts       = type.numberOutputProducts();
		spectra_data_size = numProducts * numTunings * numFreqsOrSpf * sizeof(float) * (isCorr ? 2 : 1);
		spectra_size      = sizeof(DrxSpectraHeader) + spectra_data_size;
		numSpectraPresent = getFileSize() / (sizeof(DrxSpectraHeader) + spectra_data_size);
		tuning1CenterFreq = ((double)temp_header.freqCode[0]) * FREQ_CODE_FACTOR;
		tuning2CenterFreq = ((double)temp_header.freqCode[1]) * FREQ_CODE_FACTOR;
		bandwidth 		  = DP_BASE_FREQ_HZ / temp_header.decFactor;
		temp_data         = (float*) MALLOC(spectra_data_size);
		if (startSpectra >= numSpectraPresent){
			cout << "Start frame ("<<startSpectra<<") is after the number of deteced spectra ("<<numSpectraPresent<<")"<< endl;
			exit(-1);
		}

		if ((startSpectra + _count) >=  numSpectraPresent){
			cout << "Warning, supplied range overlaps the end of the file, truncating spectra count to match"<< endl;
			numSpectraUsed = numSpectraPresent - startSpectra;
		} else {
			numSpectraUsed    = _count;
		}
		start = startSpectra;

		if (isCorr){
			cbuf = new CorrBuffer(numSpectraUsed,numTunings,numFreqsOrSpf,numProducts,numInts,logScale);
			if (!cbuf){
				cout << "Allocation failure.\n";
				exit(-1);
			}
			cbuf->printSpectraHeader(&temp_header);
		} else {
			sbuf = new SpectraBuffer(numSpectraUsed,numTunings,numFreqsOrSpf,numProducts,numInts,logScale);
			if (!sbuf){
				cout << "Allocation failure.\n";
				exit(-1);
			}
			sbuf->printSpectraHeader(&temp_header);
		}



	}
	void readall(){
		lseek(fd,spectra_size * start,SEEK_SET); // reset position
		for (size_t i=0; i<numSpectraUsed; i++){
			if (! findZeros) cout << "Reading frame " << i << std::endl;
			readHeader();
			//buf->printSpectraHeader(&temp_header);
			if (isCorr){
				if (cbuf->checkCompatible(&temp_header)){
					readData();
					cbuf->unpack(i,&temp_header,temp_data,findZeros);
				} else {
					cout << "\n========================================================\n";
					report();
					cout << "========================================================\n";
					cbuf->printSpectraHeader(&temp_header);
					cout << "========================================================\n";
					cout << "Frame geometry/type/etc changed at frame index "<<(start+i)<<"\n";
					exit(-1);
				}
			} else {
				if (sbuf->checkCompatible(&temp_header)){
					readData();
					sbuf->unpack(i,&temp_header,temp_data,findZeros);
				} else {
					cout << "\n========================================================\n";
					report();
					cout << "========================================================\n";
					sbuf->printSpectraHeader(&temp_header);
					cout << "========================================================\n";
					cout << "Frame geometry/type/etc changed at frame index "<<(start+i)<<"\n";
					exit(-1);
				}
			}
		}
		//exit(-1);
		cout << "\n";
		cout << "Read "<<numSpectraUsed<<" spectra\n";
		close(fd);
	}
	SpectraBuffer*   getSBuf(){
		return sbuf;
	}
	CorrBuffer*      getCBuf(){
		return cbuf;
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
	void readIt(char * buf, size_t size){
		ssize_t res=0;
		size_t readSoFar=0;
		while((readSoFar < size) && ((res=read(fd,&buf[readSoFar],size-readSoFar))>=0)){
			readSoFar+=(size_t)res;
		}
		if (res<0){
			cerr << "\nI/O error\n";
			close(fd);
			exit(0);
		}
	}
	void readHeader(){
		readIt((char*)&temp_header,sizeof(DrxSpectraHeader));
	}
	void readData(){
		readIt((char*)temp_data,spectra_data_size);

	}

	void report(){
		cout << "spectra_data_size: " << spectra_data_size << endl;
		cout << "spectra_size:      " << spectra_size << endl;
		cout << "type:              " << (isCorr ? ("Correlation:"+type.name()) : ("Spectrogram:"+type.name())) << endl;
		cout << "start:             " << start << endl;
		cout << "numSpectraPresent: " << numSpectraPresent << endl;
		cout << "numSpectraUsed:    " << numSpectraUsed << endl;
		cout << "numFreqs:          " << numFreqsOrSpf << endl;
		cout << "numTunings:        " << numTunings << endl;
		cout << "numProducts:       " << numProducts << endl;
		cout << "numInts:           " << numInts << endl;
		cout << "tuning1CenterFreq: " << tuning1CenterFreq/MHz << endl;
		cout << "tuning2CenterFreq: " << tuning2CenterFreq/MHz << endl;
		cout << "bandwidth:         " << bandwidth/MHz << endl;


	}
	size_t           getSpectra_data_size(){return spectra_data_size;}
	size_t           getSpectra_size(){return spectra_size;}
	ProductType      getType(){return type;}
	size_t           getStart(){return start;}
	size_t           getNumSpectraPresent(){return numSpectraPresent;}
	size_t           getNumSpectraUsed(){return numSpectraUsed;}
	size_t           getNumFreqsOrSpf(){return numFreqsOrSpf;}
	size_t           getNumTunings(){return numTunings;}
	size_t           getNumProducts(){return numProducts;}
	size_t           getNumInts(){return numInts;}
	double           getTuning1CenterFreq(){return tuning1CenterFreq;}
	double           getTuning2CenterFreq(){return tuning2CenterFreq;}
	double           getBandwidth(){return bandwidth;}
	bool             isCorrelation(){return isCorr;}
private:
	bool             findZeros;
	bool             logScale;
	int              fd;
	SpectraBuffer*   sbuf;
	CorrBuffer*      cbuf;
	DrxSpectraHeader temp_header;
	float*           temp_data;
	size_t           spectra_data_size;
	size_t           spectra_size;
	ProductType      type;
	size_t           start;
	size_t           numSpectraPresent;
	size_t           numSpectraUsed;
	size_t           numFreqsOrSpf;
	size_t           numTunings;
	size_t           numProducts;
	size_t           numInts;
	double           tuning1CenterFreq;
	double           tuning2CenterFreq;
	double           bandwidth;
	bool             isCorr;

};


void plotWaterfall(string groupTitle, string name, string xlabel, string ylabel, string zlabel, float* data, size_t xdim, double xmin, double xmax, size_t ydim, double ymin, double ymax){
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set size 0.95,0.95;");
	plotter0.set_xrange(xmin,xmax);
	plotter0.set_yrange(ymin,ymax);
	plotter0.set_title(name);
	plotter0.set_xlabel(xlabel);
	plotter0.set_ylabel(ylabel);
	plotter0.cmd("set cblabel \""+zlabel+"\"");
	plotter0.plot_imagef(data, xmin, xmax, xdim, ymin, ymax, ydim, "");
}

void plotDualWaterfall(
		string groupTitle,
		string name0, string name1,
		string xlabel, string ylabel, string zlabel,
		float* data0, float* data1,
		size_t xdim, double xmin, double xmax, size_t ydim, double ymin, double ymax){
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.45,0.95;");

	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name0);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		//plotter0.cmd("set cblabel \""+zlabel+"\"");
		plotter0.plot_imagef(data0, xmin, xmax, xdim, ymin, ymax, ydim, "");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name1);
		plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.cmd("set cblabel \""+zlabel+"\"");
		plotter0.plot_imagef(data1, xmin, xmax, xdim, ymin, ymax, ydim, "");
	plotter0.cmd("unset multiplot");
}

void plotQuadWaterfall(
		string groupTitle,
		string name0, string name1, string name2, string name3,
		string xlabel, string ylabel, string zlabel,
		float* data0, float* data1, float* data2, float* data3,
		size_t xdim, double xmin, double xmax, size_t ydim, double ymin, double ymax){
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.45,0.45;");

	plotter0.cmd("set origin 0.0,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name0);
		//plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		//plotter0.cmd("set cblabel \""+zlabel+"\"");
		plotter0.plot_imagef(data0, xmin, xmax, xdim, ymin, ymax, ydim, "");
	plotter0.cmd("set origin 0.5,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name1);
		//plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.cmd("set cblabel \""+zlabel+"\"");
		plotter0.plot_imagef(data1, xmin, xmax, xdim, ymin, ymax, ydim, "");
	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name2);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		//plotter0.cmd("set cblabel \""+zlabel+"\"");
		plotter0.plot_imagef(data2, xmin, xmax, xdim, ymin, ymax, ydim, "");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name3);
		plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.cmd("set cblabel \""+zlabel+"\"");
		plotter0.plot_imagef(data3, xmin, xmax, xdim, ymin, ymax, ydim, "");
	plotter0.cmd("unset multiplot");
}
template <class T>
void plotSeries(
		string groupTitle,
		string name,
		string xlabel, string ylabel,
		T* data,
		size_t count,
		T xmin, T xmax,
		T ymin, T ymax,
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y(count);
	double r=xmax-xmin;
	for (size_t c=0; c<count; c++){
		x[c] = xmin + r * c/count;
		y[c] = data[c];
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.set_xrange(xmin,xmax);
	plotter0.set_yrange(ymin,ymax);
	plotter0.set_title(name);
	plotter0.set_xlabel(xlabel);
	plotter0.set_ylabel(ylabel);
	plotter0.set_style(style);
	plotter0.plot_xy(x,y,"");
}

template <class T>
void plotDualSeries(
		string groupTitle,
		string name0, string name1,
		string xlabel, string ylabel,
		T* data0, T* data1,
		size_t count,
		T xmin, T xmax,
		T ymin, T ymax,
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y0(count);
	vector<float> y1(count);
/*	vector<float> y2(count);
	vector<float> y3(count);*/
	double r=xmax-xmin;
	for (size_t c=0; c<count; c++){
		x[c] = xmin + r * c/count;
		y0[c] = data0[c];
		y1[c] = data1[c];
/*		y2[c] = data2[c];
		y3[c] = data3[c];*/
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.5,1.0;");
	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name0);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y0,"");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name1);
		plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y1,"");
	plotter0.cmd("unset multiplot");
}

template <class T>
void plotQuadSeries(
		string groupTitle,
		string name0, string name1, string name2, string name3,
		string xlabel, string ylabel,
		T* data0, T* data1, T* data2, T* data3,
		size_t count,
		T xmin, T xmax,
		T ymin, T ymax,
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y0(count);
	vector<float> y1(count);
	vector<float> y2(count);
	vector<float> y3(count);
	double r=xmax-xmin;
	for (size_t c=0; c<count; c++){
		x[c] = xmin + r * c/count;
		y0[c] = data0[c];
		y1[c] = data1[c];
		y2[c] = data2[c];
		y3[c] = data3[c];
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.5,0.5;");
	plotter0.cmd("set origin 0.0,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name0);
		//plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y0,"");
	plotter0.cmd("set origin 0.5,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name1);
		//plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y1,"");
	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name2);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y2,"");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name3);
		plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y3,"");
	plotter0.cmd("unset multiplot");
}
template <class Tx, class Ty>
void plotSeriesXY(
		string groupTitle,
		string name,
		string xlabel, string ylabel,
		size_t count,
		Tx* x_data,
		Tx xmin, Tx xmax,
		Ty* y_data,
		Ty ymin, Ty ymax,
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y(count);
	for (size_t c=0; c<count; c++){
		x[c] = x_data[c];
		y[c] = y_data[c];
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.set_xrange(xmin,xmax);
	plotter0.set_yrange(ymin,ymax);
	plotter0.set_title(name);
	plotter0.set_xlabel(xlabel);
	plotter0.set_ylabel(ylabel);
	plotter0.set_style(style);
	plotter0.plot_xy(x,y,"");
}

template <class Tx, class Ty>
void plotDualSeriesXY(
		string groupTitle,
		string name0, string name1,
		string xlabel, string ylabel,
		size_t count,
		Tx* x_data,
		Tx xmin, Tx xmax,
		Ty* y0_data, Ty* y1_data,
		Ty ymin, Ty ymax,
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y0(count);
	vector<float> y1(count);
	for (size_t c=0; c<count; c++){
		x[c]  = x_data[c];
		y0[c] = y0_data[c];
		y1[c] = y1_data[c];
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.5,1.0;");
	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name0);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y0,"");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name1);
		plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y1,"");
	plotter0.cmd("unset multiplot");
}

template <class Tx, class Ty>
void plotQuadSeriesXY(
		string groupTitle,
		string name0, string name1, string name2, string name3,
		string xlabel, string ylabel,
		size_t count,
		Tx* x_data,
		Tx xmin, Tx xmax,
		Ty* y0_data, Ty* y1_data, Ty* y2_data, Ty* y3_data,
		Ty ymin, Ty ymax,
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y0(count);
	vector<float> y1(count);
	vector<float> y2(count);
	vector<float> y3(count);
	for (size_t c=0; c<count; c++){
		x[c]  = x_data[c];
		y0[c] = y0_data[c];
		y1[c] = y1_data[c];
		y2[c] = y2_data[c];
		y3[c] = y3_data[c];
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.5,0.5;");
	plotter0.cmd("set origin 0.0,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name0);
		//plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y0,"");
	plotter0.cmd("set origin 0.5,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name1);
		//plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y1,"");
	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name2);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y2,"");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin,ymax);
		plotter0.set_title(name3);
		plotter0.set_xlabel(xlabel);
		//plotter0.set_ylabel(ylabel);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y3,"");
	plotter0.cmd("unset multiplot");
}

template <class Tx, class Ty>
void plotQuadSeriesXY2(
		string groupTitle,
		string names[4],
		string xlabel, string ylabel[4],
		size_t count,
		Tx* x_data,
		Tx xmin, Tx xmax,
		Ty* ydata[4],
		Ty ymin[4], Ty ymax[4],
		string style="linespoints"){
	vector<float> x(count);
	vector<float> y0(count);
	vector<float> y1(count);
	vector<float> y2(count);
	vector<float> y3(count);
	for (size_t c=0; c<count; c++){
		x[c]  = x_data[c];
		y0[c] = ydata[0][c];
		y1[c] = ydata[1][c];
		y2[c] = ydata[2][c];
		y3[c] = ydata[3][c];
	}
	Gnuplot plotter0(true);
	plotter0.cmd("set term x11 title '"+groupTitle+"';");
	plotter0.cmd("set multiplot;");
	plotter0.cmd("set size 0.5,0.5;");
	plotter0.cmd("set origin 0.0,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin[0],ymax[0]);
		plotter0.set_title(names[0]);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel[0]);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y0,"");
	plotter0.cmd("set origin 0.5,0.5");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin[1],ymax[1]);
		plotter0.set_title(names[1]);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel[1]);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y1,"");
	plotter0.cmd("set origin 0.0,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin[2],ymax[2]);
		plotter0.set_title(names[2]);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel[2]);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y2,"");
	plotter0.cmd("set origin 0.5,0.0");
		plotter0.set_xrange(xmin,xmax);
		plotter0.set_yrange(ymin[3],ymax[3]);
		plotter0.set_title(names[3]);
		plotter0.set_xlabel(xlabel);
		plotter0.set_ylabel(ylabel[3]);
		plotter0.set_style(style);
		plotter0.plot_xy(x,y3,"");
	plotter0.cmd("unset multiplot");
}


int main(int argc, char* argv[])
{
	if (argc<4) {
		cout << "Error: invalid # of args("<<argc<<"). \nUsage:\n\t";
		cout << argv[0] << " <filename> <start(1st spectra index)> <length(# spectra; -1 means until end)>\n";
		exit(-1);
	}
	string options((argc>4)?argv[4]:"sp,tp,is");
	cout << "Options: " << options << endl;
	bool findZeros      = ((options.find("findZeros"         )!=string::npos)  || (options.find("fz")!=string::npos));
	bool showWaterfalls = ((options.find("spectrograms"      )!=string::npos)  || (options.find("sp")!=string::npos));
	bool showPower      = ((options.find("total-power"       )!=string::npos)  || (options.find("tp")!=string::npos));
	bool showIntSpec    = ((options.find("integrated-spectra")!=string::npos)  || (options.find("is")!=string::npos));
	bool showFills      = ((options.find("fills"             )!=string::npos)  || (options.find("f")!=string::npos));
	bool showSats       = ((options.find("sat-counts"        )!=string::npos)  || (options.find("sc")!=string::npos));
	bool showErr        = ((options.find("errors"            )!=string::npos)  || (options.find("er")!=string::npos));
	bool logScale       = ((options.find("log"               )!=string::npos)  || (options.find("lg")!=string::npos));
	bool showTimeTags   = ((options.find("timetags"          )!=string::npos)  || (options.find("tt")!=string::npos));
	SpecReader sr(string(argv[1]),strtoul(argv[2],NULL,10),strtol(argv[3],NULL,10), logScale, findZeros);
	sr.report();
	cout << flush;
	sr.readall();

	if (findZeros){
		cout << "FINDZEROS: done...\n";
		exit(0);
	}
	if (sr.isCorrelation()){
		string powerAxisLabel = logScale ? "Power (dB)" : "Power (linear)";

		double numFrames = sr.getNumSpectraUsed();
		size_t nS        = sr.getNumSpectraUsed();
		double numSpf    = sr.getNumFreqsOrSpf();
		size_t nSpf      = sr.getNumFreqsOrSpf();

		size_t nTime     = nS;
		size_t nTimeFull = nS * nSpf;

		double numInts  = sr.getNumInts();
		double centerFreq[2];


		centerFreq[0]=sr.getTuning1CenterFreq()/MHz;
		centerFreq[1]=sr.getTuning2CenterFreq()/MHz;

		double* times = sr.getCBuf()->timePtr();
		double* timesFull = sr.getCBuf()->timeFullPtr();
		double timeMin  = 0;
		double timeMax  = sr.getCBuf()->getMaxTime();



		CorrProduct type = sr.getType().toCorr();
		string tnames[2];
		tnames[0]="Tuning 0";
		tnames[1]="Tuning 1";

		string filename = string(argv[1]);
		string basename = filename;
		size_t n;
		while ((n = basename.find('/')) != string::npos){
			//cout << basename << endl;
			basename = basename.erase(0,n+1);
		}
		string titlePrefix = "SpectrogramViewer ("+basename+") --- ";
		string groupTitle;


		//           ____________
		//           |real, imag|
		// format is |pow, angle|, 1 for each product
		//           |__________|
		for(size_t p=0; p<sr.getNumProducts();p++){
			for (size_t t =0; t<sr.getNumTunings(); t++){
				groupTitle = titlePrefix + tnames[t] + " " +getProductName(type,p);


				// time axis limits
				double mxt = sr.getCBuf()->getMaxFullTime();
				double mnt = sr.getCBuf()->getMinFullTime();


				// power range
				double mxp = sr.getCBuf()->getMaxPwr(t,p);
				double mnp = sr.getCBuf()->getMinPwr(t,p);
				if (!mnp && !mxp){mnp=-1; mxp=1;}
				if (mnp==mxp){mxp=1.05*mnp;}

				// angle range
				double mxa = sr.getCBuf()->getMaxAngle();
				double mna = sr.getCBuf()->getMinAngle();


				// real range
				double mxr = sr.getCBuf()->getMaxReal(t,p);
				double mnr = sr.getCBuf()->getMinReal(t,p);
				if (!mnr && !mxr){mnr=-1; mxr=1;}
				if (mnr==mxr){mxr=1.05*mnr;}

				// imag range
				double mxi = sr.getCBuf()->getMaxImag(t,p);
				double mni = sr.getCBuf()->getMinImag(t,p);
				if (!mni && !mxi){mni=-1; mxi=1;}
				if (mni==mxi){mxi=1.05*mni;}

				string titles[4] = {
					tnames[t] + " Re["+getProductName(type,p)+"]",
					tnames[t] + " Im["+getProductName(type,p)+"]",
					tnames[t] + " |"+getProductName(type,p)+"|",
					tnames[t] + " phase("+getProductName(type,p)+")"
				};
				double ymin[4] = { mnr, mni, mnp, mna};
				double ymax[4] = { mxr, mxi, mxp, mxa};
				double* data[4]={
						sr.getCBuf()->where(0,t,0,p,true),
						sr.getCBuf()->where(0,t,0,p,false),
						sr.getCBuf()->powPtr(t,p,0,0),
						sr.getCBuf()->anglePtr(t,p,0,0)
				};

				string ylabels[4] = {
						"Avg Real (linear, arb.)",
						"Avg Imag (linear, arb.)",
						powerAxisLabel,
						"Phase (radians)"
				};
				plotQuadSeriesXY2(
					groupTitle, titles, "Time (s)", ylabels,
					nTimeFull,
					timesFull, mnt,  mxt,
					data,      ymin, ymax
				);
			}
		}

		if (showTimeTags){
			groupTitle  =  titlePrefix + "Time Tags";
			// plot time tags
			double mnt_ = sr.getCBuf()->getMinTime();
			double mxt_ = sr.getCBuf()->getMaxTime();
			plotSeries(
					groupTitle,
					"Time tags ",
					"Frame index", "Timetag",
					times,
					nS,
					(double)0,(double)(nS-1),
					mnt_,mxt_
			);
		}

		if (showFills){
			groupTitle  =  titlePrefix + "Fill Counts";

			// plot fills
			double mnf[4];
			double mxf[4];
			double mnf_ = numeric_limits<double>::max();
			double mxf_ = (-numeric_limits<double>::max());
			for (size_t s =0; s<4; s++){
				mnf[s] = sr.getCBuf()->getMinFill(s);
				if (mnf[s] < mnf_) mnf_ = mnf[s];
				mxf[s] = sr.getCBuf()->getMaxFill(s);
				if (mxf[s] > mxf_) mxf_ = mxf[s];
			}
			if ((mxf_ - mnf_) < numeric_limits<double>::epsilon()){
				mnf_ = 0;
				mxf_ = 1;
			}
			mnf_=min(0.0,mnf_);
			mxf_=max(1.1,mxf_);
			plotQuadSeriesXY(
					groupTitle,
					"Fills: " + tnames[0] + " X",
					"Fills: " + tnames[0] + " Y",
					"Fills: " + tnames[1] + " X",
					"Fills: " + tnames[1] + " Y",
					"Time (s)", "Fill (count)",
					nS,
					times,
					timeMin,timeMax,
					sr.getCBuf()->fillPtr(0,0),
					sr.getCBuf()->fillPtr(0,1),
					sr.getCBuf()->fillPtr(0,2),
					sr.getCBuf()->fillPtr(0,3),
					mnf_,mxf_
			);
		}

		if (showSats){
			groupTitle  =  titlePrefix + "Saturation Counts";
			// plot sat counts
			double mns[4];
			double mxs[4];
			double mns_ = numeric_limits<double>::max();
			double mxs_ = (-numeric_limits<double>::max());
			for (size_t s =0; s<4; s++){
				mns[s] = sr.getCBuf()->getMinSat(s);
				if (mns[s] < mns_) mns_ = mns[s];
				mxs[s] = sr.getCBuf()->getMaxSat(s);
				if (mxs[s] > mxs_) mxs_ = mxs[s];
			}
			if ((mxs_ - mns_) < numeric_limits<double>::epsilon()){
				mns_ = 0;
				mxs_ = 1;
			}
			mns_=min(0.0,mns_);
			mxs_=max(1.1,mxs_);
			plotQuadSeriesXY(
					groupTitle,
					"Saturation Count: " + tnames[0] + " X",
					"Saturation Count: " + tnames[0] + " Y",
					"Saturation Count: " + tnames[1] + " X",
					"Saturation Count: " + tnames[1] + " Y",
					"Time (s)", "Saturation Count (count)",
					nS,
					times,
					timeMin,timeMax,
					sr.getCBuf()->satPtr(0,0),
					sr.getCBuf()->satPtr(0,1),
					sr.getCBuf()->satPtr(0,2),
					sr.getCBuf()->satPtr(0,3),
					mns_,mxs_
			);
		}

		if (showErr){
			groupTitle  =  titlePrefix + "Errors";
			// plot errors
			float mne_ = -0.1;
			float mxe_ = 1;
			plotQuadSeriesXY(
					groupTitle,
					"Error Flag: " + tnames[0] + " X",
					"Error Flag: " + tnames[0] + " Y",
					"Error Flag: " + tnames[1] + " X",
					"Error Flag: " + tnames[1] + " Y",
					"Time (s)", "Error Flag (true/false)",
					nS,
					times,
					timeMin,timeMax,
					sr.getCBuf()->errPtr(0,0),
					sr.getCBuf()->errPtr(0,1),
					sr.getCBuf()->errPtr(0,2),
					sr.getCBuf()->errPtr(0,3),
					mne_,mxe_
			);
		}

	} else {


		string powerAxisLabel = logScale ? "Power (dB)" : "Power (linear)";

		//double numSpecs = sr.getNumSpectraUsed();
		size_t nS = sr.getNumSpectraUsed();
		double numFreqs = sr.getNumFreqsOrSpf();
		size_t nF = sr.getNumFreqsOrSpf();

		//double numInts  = sr.getNumInts();
		double centerFreq[2];
		double bw = sr.getBandwidth();

		double frange = (2*bw/numFreqs)/MHz;
		double minfreq[2];
		double maxfreq[2];

		centerFreq[0]=sr.getTuning1CenterFreq()/MHz;
		centerFreq[1]=sr.getTuning2CenterFreq()/MHz;
		minfreq[0] = centerFreq[0] - frange * (numFreqs/2);
		minfreq[1] = centerFreq[1] - frange * (numFreqs/2);
		maxfreq[0] = centerFreq[0] + frange * ((numFreqs/2)-1);
		maxfreq[1] = centerFreq[1] + frange * ((numFreqs/2)-1);

		//double timestep = ((numFreqs * numInts)/bw);
		double* times = sr.getSBuf()->timePtr();
		double timeMin  = 0;
		double timeMax  = sr.getSBuf()->getMaxTime();



		StokesProduct type = sr.getType().toStokes();
		string tnames[2];
		tnames[0]="Tuning 0";
		tnames[1]="Tuning 1";

		string filename = string(argv[1]);
		string basename = filename;
		size_t n;
		while ((n = basename.find('/')) != string::npos){
			//cout << basename << endl;
			basename = basename.erase(0,n+1);
		}
		string titlePrefix = "SpectrogramViewer ("+basename+") --- ";
		string groupTitle;
		if(showWaterfalls){
			groupTitle  =  titlePrefix + "Spectrogram";
			// plot base spectra waterfalls
			for (size_t t =0; t<sr.getNumTunings(); t++){
				switch(type){
				case IV:
				case XXYY:
					plotDualWaterfall(
							groupTitle,
							tnames[t]+" " + getProductName(type,0),
							tnames[t]+" " + getProductName(type,1),
							"Frequency (MHz)", "Time (s)", powerAxisLabel,
							sr.getSBuf()->where(0,t,0,0),
							sr.getSBuf()->where(0,t,0,1),
							nF,minfreq[t],maxfreq[t],
							nS,timeMin,timeMax
					); break;
				case IQUV:
					plotQuadWaterfall(
							groupTitle,
							tnames[t]+" " + getProductName(type,0),
							tnames[t]+" " + getProductName(type,1),
							tnames[t]+" " + getProductName(type,2),
							tnames[t]+" " + getProductName(type,3),
							"Frequency (MHz)", "Time (s)", powerAxisLabel,
							sr.getSBuf()->where(0,t,0,0),
							sr.getSBuf()->where(0,t,0,1),
							sr.getSBuf()->where(0,t,0,2),
							sr.getSBuf()->where(0,t,0,3),
							nF,minfreq[t],maxfreq[t],
							nS,timeMin,timeMax
					); break;
				default:
					cout << "unsupported stokes type" << endl;
					exit(-1);
				}
			}
		}

		if(showIntSpec){
			groupTitle  =  titlePrefix + "Integrated Spectra";
			// plot integrated spectra
			for (size_t t =0; t<sr.getNumTunings(); t++){
				double mxi[4];
				double mni[4];
				double mni_ = numeric_limits<double>::max();
				double mxi_ = (-numeric_limits<double>::max());
				for(size_t p=0; p<sr.getNumProducts();p++){
					mxi[p]= sr.getSBuf()->getMaxInt(t,p);
					if (mxi[p] > mxi_) mxi_ = mxi[p];
					mni[p]= sr.getSBuf()->getMinInt(t,p);
					if (mni[p] < mni_) mni_ = mni[p];
				}
				if ((mxi_ - mni_) < numeric_limits<double>::epsilon()){
					mni_ = 0;
					mxi_ = 1;
				}

				switch(type){
				case IV:
				case XXYY:
					plotDualSeries(
							groupTitle,
							"Integrated Spectra for " + tnames[t]+" " + getProductName(type,0),
							"Integrated Spectra for " + tnames[t]+" " + getProductName(type,1),
							"Frequency (MHz)", powerAxisLabel,
							sr.getSBuf()->intPtr(t,0,0),
							sr.getSBuf()->intPtr(t,1,0),
							nF,minfreq[t],maxfreq[t],
							mni_,mxi_
					); break;
				case IQUV:
					plotQuadSeries(
							groupTitle,
							"Integrated Spectra for " + tnames[t]+" " + getProductName(type,0),
							"Integrated Spectra for " + tnames[t]+" " + getProductName(type,1),
							"Integrated Spectra for " + tnames[t]+" " + getProductName(type,2),
							"Integrated Spectra for " + tnames[t]+" " + getProductName(type,3),
							"Frequency (MHz)", powerAxisLabel,
							sr.getSBuf()->intPtr(t,0,0),
							sr.getSBuf()->intPtr(t,1,0),
							sr.getSBuf()->intPtr(t,2,0),
							sr.getSBuf()->intPtr(t,3,0),
							nF,minfreq[t],maxfreq[t],
							mni_,mxi_
					); break;
				default:
					cout << "unsupported stokes type" << endl;
					exit(-1);
				}
			}
		}

		if (showPower){
			groupTitle  =  titlePrefix + "Time Series Total Power";
			// plot time series total power
			for (size_t t =0; t<sr.getNumTunings(); t++){
				double mxp[4];
				double mnp[4];
				double mnp_ = numeric_limits<double>::max();
				double mxp_ = (-numeric_limits<double>::max());
				for(size_t p=0; p<sr.getNumProducts();p++){
					mxp[p]= sr.getSBuf()->getMaxPwr(t,p);
					if (mxp[p] > mxp_) mxp_ = mxp[p];
					mnp[p]= sr.getSBuf()->getMinPwr(t,p);
					if (mnp[p] < mnp_) mnp_ = mnp[p];
				}
				if ((mxp_ - mnp_) < numeric_limits<double>::epsilon()){
					mnp_ = 0;
					mxp_ = 1;
				}

				switch(type){
				case IV:
				case XXYY:
					plotDualSeriesXY(
							groupTitle,
							"Total Power for " + tnames[t]+" " + getProductName(type,0),
							"Total Power for " + tnames[t]+" " + getProductName(type,1),
							"Frequency (MHz)", powerAxisLabel,
							nS,
							times,
							timeMin,timeMax,
							sr.getSBuf()->powPtr(t,0,0),
							sr.getSBuf()->powPtr(t,1,0),
							mnp_,mxp_
					); break;
				case IQUV:
					plotQuadSeriesXY(
							groupTitle,
							"Total Power for " + tnames[t]+" " + getProductName(type,0),
							"Total Power for " + tnames[t]+" " + getProductName(type,1),
							"Total Power for " + tnames[t]+" " + getProductName(type,2),
							"Total Power for " + tnames[t]+" " + getProductName(type,3),
							"Time (s)", powerAxisLabel,
							nS,
							times,
							timeMin,timeMax,
							sr.getSBuf()->powPtr(t,0,0),
							sr.getSBuf()->powPtr(t,1,0),
							sr.getSBuf()->powPtr(t,2,0),
							sr.getSBuf()->powPtr(t,3,0),
							mnp_,mxp_
					); break;
				default:
					cout << "unsupported stokes type" << endl;
					exit(-1);
				}
			}
		}
		if (showTimeTags){
			groupTitle  =  titlePrefix + "Time Tags";
			// plot time tags
			double mnt_ = sr.getSBuf()->getMinTime();
			double mxt_ = sr.getSBuf()->getMaxTime();
			plotSeries(
					groupTitle,
					"Time tags ",
					"Frame index", "Timetag",
					times,
					nS,
					(double)0,(double)(nS-1),
					mnt_,mxt_
			);
		}

		if (showFills){
			groupTitle  =  titlePrefix + "Fill Counts";

			// plot fills
			double mnf[4];
			double mxf[4];
			double mnf_ = numeric_limits<double>::max();
			double mxf_ = (-numeric_limits<double>::max());
			for (size_t s =0; s<4; s++){
				mnf[s] = sr.getSBuf()->getMinFill(s);
				if (mnf[s] < mnf_) mnf_ = mnf[s];
				mxf[s] = sr.getSBuf()->getMaxFill(s);
				if (mxf[s] > mxf_) mxf_ = mxf[s];
			}
			if ((mxf_ - mnf_) < numeric_limits<double>::epsilon()){
				mnf_ = 0;
				mxf_ = 1;
			}
			mnf_=min(0.0,mnf_);
			mxf_=max(1.1,mxf_);
			plotQuadSeriesXY(
					groupTitle,
					"Fills: " + tnames[0] + " X",
					"Fills: " + tnames[0] + " Y",
					"Fills: " + tnames[1] + " X",
					"Fills: " + tnames[1] + " Y",
					"Time (s)", "Fill (count)",
					nS,
					times,
					timeMin,timeMax,
					sr.getSBuf()->fillPtr(0,0),
					sr.getSBuf()->fillPtr(0,1),
					sr.getSBuf()->fillPtr(0,2),
					sr.getSBuf()->fillPtr(0,3),
					mnf_,mxf_
			);
		}

		if (showSats){
			groupTitle  =  titlePrefix + "Saturation Counts";
			// plot sat counts
			double mns[4];
			double mxs[4];
			double mns_ = numeric_limits<double>::max();
			double mxs_ = (-numeric_limits<double>::max());
			for (size_t s =0; s<4; s++){
				mns[s] = sr.getSBuf()->getMinSat(s);
				if (mns[s] < mns_) mns_ = mns[s];
				mxs[s] = sr.getSBuf()->getMaxSat(s);
				if (mxs[s] > mxs_) mxs_ = mxs[s];
			}
			if ((mxs_ - mns_) < numeric_limits<double>::epsilon()){
				mns_ = 0;
				mxs_ = 1;
			}
			mns_=min(0.0,mns_);
			mxs_=max(1.1,mxs_);
			plotQuadSeriesXY(
					groupTitle,
					"Saturation Count: " + tnames[0] + " X",
					"Saturation Count: " + tnames[0] + " Y",
					"Saturation Count: " + tnames[1] + " X",
					"Saturation Count: " + tnames[1] + " Y",
					"Time (s)", "Saturation Count (count)",
					nS,
					times,
					timeMin,timeMax,
					sr.getSBuf()->satPtr(0,0),
					sr.getSBuf()->satPtr(0,1),
					sr.getSBuf()->satPtr(0,2),
					sr.getSBuf()->satPtr(0,3),
					mns_,mxs_
			);
		}

		if (showErr){
			groupTitle  =  titlePrefix + "Errors";
			// plot errors
			float mne_ = -0.1;
			float mxe_ = 1;
			plotQuadSeriesXY(
					groupTitle,
					"Error Flag: " + tnames[0] + " X",
					"Error Flag: " + tnames[0] + " Y",
					"Error Flag: " + tnames[1] + " X",
					"Error Flag: " + tnames[1] + " Y",
					"Time (s)", "Error Flag (true/false)",
					nS,
					times,
					timeMin,timeMax,
					sr.getSBuf()->errPtr(0,0),
					sr.getSBuf()->errPtr(0,1),
					sr.getSBuf()->errPtr(0,2),
					sr.getSBuf()->errPtr(0,3),
					mne_,mxe_
			);
		}
	}










/*



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
				plotter1.set_ylabel(powerAxisLabel);
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
				plotter2.set_ylabel(powerAxisLabel);
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
    */
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
