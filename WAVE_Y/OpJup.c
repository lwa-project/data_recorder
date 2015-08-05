/*
 * OpJup.c
 *
 *  Created on: Sep 18, 2011
 *      Author: chwolfe2
 */

#include "OpJup.h"
#include "Globals.h"
#include "FileSystem.h"
#include <fftw3.h>
#include <sys/statvfs.h>
#include <string.h>


#define DRX_HEADER_SIZE  		24l
#define DRX_SAMPLES_PER_PACKET	4096l
#define DRX_BYTES_PER_SAMPLE	1l


size_t inputTagfileFileSize	 = 0;
size_t inputPacketCount		 = 0;
size_t inputChunkSize		 = 0;
size_t inputTotalBytesToRead = 0;
size_t inputTotalBytesRead	 = 0;
size_t readChunkSize		 = 0;
size_t fileBasePos			 = 0;

size_t readBytesToInsert	 = 0;
size_t readBytesInsertable	 = 0;
size_t readBytesToRemove	 = 0;
size_t readBytesRemovable	 = 0;

ssize_t bytesRead			 = 0;
ssize_t bytesWritten		 = 0;
ssize_t outputBufferSize 	 = 0;
ssize_t stokesBufferSize 	 = 0;

RingQueue jupiterReadQueue;

static fftw_plan plan;
static fftw_complex* fftw_input_buffer;
static fftw_complex* fftw_output_buffer;
char* insertionPointer;
char* removalPointer;
fftw_complex* outputBuffer;
fftw_complex* stokesBuffer;


double LUT[16]={ 0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f, -8.0f, -7.0f, -6.0f, -5.0f, -4.0f, -3.0f, -2.0f, -1.0f};

static inline void fourBitToDouble(unsigned char* c, double* d){
	*d=LUT[*c];
}

static inline void eightBitToComplex(unsigned char* c, fftw_complex* d){
	unsigned char temp;
	temp = (*c & 0x0f);				fourBitToDouble(&temp, &((*d)[0]));
	temp = ((*c & 0xf0) >> 4);		fourBitToDouble(&temp, &((*d)[1]));
}

static inline void unpackSamples(unsigned char* sampleIn, fftw_complex * sampleOut, size_t count){
	size_t i;
	for (i=0; i<count; i++)
		eightBitToComplex(sampleIn + i, sampleOut + i);
}

static inline void unpackPackets(unsigned char* packetsIn, fftw_complex * packetsOut, size_t count){
	size_t i;
	size_t packetSize=DRX_HEADER_SIZE+DRX_SAMPLES_PER_PACKET;
	for (i=0; i<count; i++)
		unpackSamples(packetsIn + (i * packetSize), packetsOut + (i * DRX_SAMPLES_PER_PACKET), DRX_SAMPLES_PER_PACKET);
}
static inline void cplx_mult(fftw_complex * a,fftw_complex * b, fftw_complex *out){
	out[0][0] = (a[0][0] * b[0][0]) - (a[0][1] * b[0][1]);
	out[0][1] = (a[0][0] * b[0][1]) + (a[0][1] * b[0][0]);
}
static inline void stokes_xcorr(fftw_complex * in, fftw_complex *out){
	size_t fBin=0;
	size_t seqId=0;
	for (seqId=0; seqId<globals.opData.IntegrationCount; seqId++){
		for (fBin=0; fBin<globals.opData.TransformLength; fBin++){
			size_t outIndex = (fBin + (seqId * globals.opData.TransformLength)) * 4;
			size_t inXIndex = (fBin + (seqId * globals.opData.TransformLength*2));
			size_t inYIndex =  inXIndex + globals.opData.TransformLength;
			// XX*
			out[outIndex + 0][0] = (in[inXIndex][0]*in[inXIndex][0]) + (in[inXIndex][1]*in[inXIndex][1]);
			out[outIndex + 0][1] = 0;
			// YY*
			out[outIndex + 1][0] = (in[inYIndex][0]*in[inYIndex][0]) + (in[inYIndex][1]*in[inYIndex][1]);
			out[outIndex + 1][1] = 0;
			// XY*
			out[outIndex + 2][0] = (in[inXIndex][0]*in[inYIndex][0]) + (in[inXIndex][1]*in[inYIndex][1]);
			out[outIndex + 2][1] = (in[inXIndex][1]*in[inYIndex][0]) - (in[inXIndex][0]*in[inYIndex][1]);
			// YX*
			out[outIndex + 3][0] = (in[inYIndex][0]*in[inXIndex][0]) + (in[inYIndex][1]*in[inXIndex][1]);
			out[outIndex + 3][1] = (in[inYIndex][1]*in[inXIndex][0]) - (in[inYIndex][0]*in[inXIndex][1]);
		}
	}
}
static inline void integrate(fftw_complex * in, fftw_complex *out){
	size_t fBin		= 0;
	size_t seqId	= 0;
	size_t stokes 	= 0;
	for (seqId=0; seqId<globals.opData.IntegrationCount; seqId++){
		for (fBin=0; fBin<globals.opData.TransformLength; fBin++){
			for (stokes=0; stokes<4; stokes++){
				size_t index = ((fBin + (seqId * globals.opData.TransformLength)) * 4) + stokes;
				size_t m = 4 * globals.opData.TransformLength;
				if (seqId == 0){
					out[index][0]  = in[index][0];
					out[index][1]  = in[index][1];
				} else {
					out[index % m][0] += in[index][0];
					out[index % m][1] += in[index][1];
				}
			}
		}
	}
}




StatusCode OpJupInit(){
	bzero((void*)&jupiterReadQueue,sizeof(RingQueue));

	globals.opData.tagfile->index=-1;
	char mountPoint[EXT_FILE_MAX_NAME_LENGTH+1];
	struct statvfs statinfoSystem;
	readChunkSize=globals.fileSystem->fileSystemHeaderData->filesystemInfo.chunkSize;

	// check that external device exists
	if(Disk_GetDiskInfo(globals.opData.opDeviceName,globals.opData.extdisk)!=SUCCESS){
		Log_Add("[OPERATION JUPITER] Error: External Storage device could not be identified.");
		return FAILURE;
	}
	// mount output partition
	if(!globals.opData.extdisk->mounted){
		if(Disk_Mount(globals.opData.extdisk)!=SUCCESS){
			Log_Add("[OPERATION JUPITER] Error: External Storage device could not be mounted.");
			return FAILURE;
		}
	}
	//stat the filesystem and see if we can open a file of the appropriate size
	sprintf(mountPoint,"/LWA_EXT%s",globals.opData.opDeviceName);
	if (statvfs(mountPoint,&statinfoSystem)){
		Log_Add("[OPERATION JUPITER] Can't Stat FileSystem.");
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	}
	// check available file size
	//if (statinfoSystem.f_frsize * statinfoSystem.f_bfree < outputFileSize){
	//	Log_Add("[OPERATION JUPITER] Insufficient storage space for jupiter operation.");
	//	return StateIfFail;
	//}

	// set file name
	sprintf(globals.opData.opCurrentFileName,"%s/%s",mountPoint,globals.opData.opFileName);

	// open output file
	globals.opData.extFileHandle = open(globals.opData.opCurrentFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (globals.opData.extFileHandle < 0){
		Log_Add("[OPERATION JUPITER] Error: Destination file '%s' could not be opened.",globals.opData.opFileName);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	} else {
		Log_Add("[OPERATION JUPITER] Opened '%s'",globals.opData.opCurrentFileName);
	}

	// open tag file
	if (FileSystem_OpenFile(globals.fileSystem,globals.opData.tagfile,globals.opData.opTag,READ)!=SUCCESS){
		Log_Add("[OPERATION JUPITER] Error: Tag file could not be opened.");
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	}
	// get tagfile filesize
	inputTagfileFileSize = globals.fileSystem->fileSystemHeaderData->fileInfo[globals.opData.tagfile->index].size;
	inputPacketCount= (
			(globals.opData.IntegrationCount * 2)         				/    		/* number of accumulations, times 2 for dual pol*/
			(DRX_SAMPLES_PER_PACKET/globals.opData.TransformLength)					/* how many accumulations per packet */
	);
	inputChunkSize =(
			inputPacketCount * (
				(DRX_SAMPLES_PER_PACKET * DRX_BYTES_PER_SAMPLE) + DRX_HEADER_SIZE 		/* packet size */
			)
	);
	inputTotalBytesToRead = ((inputTagfileFileSize / inputChunkSize) * inputChunkSize);
	fileBasePos =
		globals.fileSystem->fileSystemHeaderData->fileInfo[globals.opData.tagfile->index].startPosition +
		globals.fileSystem->fileSystemHeaderData->filesystemInfo.chunkSize;

	// allocate bufferspace for computing full stokes cross correlations
	stokesBufferSize =
		globals.opData.TransformLength *
		globals.opData.IntegrationCount *
		4 * /*   XX* YY* XY* YX*  */
		sizeof(fftw_complex)
	;
	stokesBuffer = (fftw_complex*) malloc(stokesBufferSize);
	if (!stokesBuffer){
		Log_Add("[OPERATION JUPITER]: Error allocating stokes buffer\n");
		FileSystem_CloseFile(globals.opData.tagfile);
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;

	}

	// allocate bufferspace for computing final integrated spectrum
	outputBufferSize = stokesBufferSize / globals.opData.IntegrationCount;
	outputBuffer = (fftw_complex*) malloc(outputBufferSize);
	if (!outputBuffer){
		Log_Add("[OPERATION JUPITER]: Error allocating output buffer\n");
		free(stokesBuffer);
		FileSystem_CloseFile(globals.opData.tagfile);
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	}



	// create read ring queue
	if (RingQueue_Create(&jupiterReadQueue,128)!=SUCCESS) {
		Log_Add("[OPERATION JUPITER]: Error creating read ring queue\n");
		free(outputBuffer);
		free(stokesBuffer);
		FileSystem_CloseFile(globals.opData.tagfile);
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	}

	// create FFT input buffer
	fftw_input_buffer = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * globals.opData.TransformLength * globals.opData.IntegrationCount);
	if (!fftw_input_buffer){
		Log_Add("[OPERATION JUPITER]: Error allocating transform input buffer memory\n");
		free(outputBuffer);
		free(stokesBuffer);
		RingQueue_Destroy(&jupiterReadQueue);
		FileSystem_CloseFile(globals.opData.tagfile);
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	}
	// create FFT output buffer
	fftw_output_buffer = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * globals.opData.TransformLength * globals.opData.IntegrationCount);
	if (!fftw_output_buffer){
		Log_Add("[OPERATION JUPITER]: Error allocating transform output buffer memory\n");
		free(outputBuffer);
		free(stokesBuffer);
		fftw_free((void*) fftw_input_buffer);
		RingQueue_Destroy(&jupiterReadQueue);
		FileSystem_CloseFile(globals.opData.tagfile);
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
	}
	// create FFT plan
	int n=(int) globals.opData.TransformLength;
    plan = fftw_plan_many_dft(
    		1,											// rank 1 (1 dimensional)
    		&n,											// 1st (only) dimension is from 0..(n[0]-1)
    		(int) globals.opData.IntegrationCount * 2,	// repeat transform integration count * 2 times
    		fftw_input_buffer,							// input data starts here
    		NULL,										// same physical and logical dimensions (input side)
    		1,											// contiguous samples
    		(int)globals.opData.TransformLength,		// consecutive transforms stored this far apart in input buffer
    		fftw_output_buffer,							// output data goes here
    		NULL,										// same physical and logical dimensions (output side)
    		1,											// contiguous samples
    		(int)globals.opData.TransformLength,		// consecutive transforms stored this far apart in output buffer
    		FFTW_FORWARD,								// FFT in the forward direction
    		FFTW_MEASURE								// measure for best performance
    );
    if (plan==NULL){
		Log_Add("[OPERATION JUPITER]: Error allocating transform output buffer memory\n");
		free(outputBuffer);
		free(stokesBuffer);
		fftw_free((void*) fftw_output_buffer);
		fftw_free((void*) fftw_input_buffer);
		RingQueue_Destroy(&jupiterReadQueue);
		FileSystem_CloseFile(globals.opData.tagfile);
		close(globals.opData.extFileHandle);
		Disk_Unmount(globals.opData.extdisk);
		return FAILURE;
    }

	// set local control variables
    readBytesToInsert	 = 0;
    readBytesInsertable	 = 0;
    readBytesToRemove	 = 0;
    readBytesRemovable	 = 0;
    bytesRead			 = 0;
    bytesWritten		 = 0;

    insertionPointer = NULL;
    removalPointer = NULL;

    // set global control variables
    globals.opData.opFileCpyDmpCurrentPos = 0;

    double fs = 19.6e6;
    double bw = fs / ((double) globals.opData.TransformLength);
    double tr = (1.0 / bw) * ((double) globals.opData.IntegrationCount);

    printf("Spectrometer geometry information\n");
    printf("=================================\n");
    printf("Channels:                                  %u\n", globals.opData.TransformLength);
    printf("Integration Count:                         %u\n", globals.opData.IntegrationCount);
    printf("Spectral Resolution                        %f (MHz)\n", (bw / 1e6 ));
    printf("Temporal Resolution                        %f (ms)\n",  (tr / 1e-3));
	printf("      ---------------------------\n");
	printf("inputTagfileFileSize                       %lu\n",inputTagfileFileSize);
	printf("fileBasePos                                %lu\n",fileBasePos);
	printf("readChunkSize                              %lu\n",readChunkSize);
	printf("      ---------------------------\n");
	printf("inputPacketCount                           %lu\n",inputPacketCount);
	printf("inputChunkSize                             %lu\n",inputChunkSize);
	printf("inputTotalBytesToRead                      %lu\n",inputTotalBytesToRead);
	printf("      ---------------------------\n");
	printf("outputBufferSize                           %lu\n",outputBufferSize);
	printf("      ---------------------------\n");
	printf("stokesBufferSize                           %lu\n",stokesBufferSize);
	printf("=================================\n");

    return SUCCESS;
}


StatusCode OpJupRun(){
	static size_t rptcntr =0;

	// read / enqueue
	if (inputTotalBytesRead < inputTotalBytesToRead){
		//printf("read cycle \n"); fflush(stdout);
		if (!insertionPointer){
			//printf("!insertionPointer \n"); fflush(stdout);
			readBytesToInsert = inputTotalBytesToRead-inputTotalBytesRead;
			if (readBytesToInsert > readChunkSize)
				readBytesToInsert = readChunkSize;
			RingQueue_NextInsertionPoint(
					&jupiterReadQueue,
					&insertionPointer,
					readChunkSize,// always read 'readChunkSize' bytes, but then only "insert" the proper amount
					&readBytesInsertable
				);
		}
		if (insertionPointer){
			//printf("insertionPointer \n"); fflush(stdout);
			if (_FileSystem_SynchronousRead(
					globals.fileSystem,
					insertionPointer,
					fileBasePos+inputTotalBytesRead,
					readChunkSize, // always read 'readChunkSize' bytes, but then only "insert" the proper amount
					&bytesRead) == SUCCESS){
				RingQueue_NotifyInsertionComplete(&jupiterReadQueue,readBytesToInsert);
				inputTotalBytesRead += readBytesToInsert;
				insertionPointer = NULL;

			}


		}
	}

	// check read queue for sufficient data to process
	//printf("process/write cycle \n"); fflush(stdout);

	if (!removalPointer){
		//printf("!removalPointer \n"); fflush(stdout);
		RingQueue_NextRemovalPoint(&jupiterReadQueue,&removalPointer,inputChunkSize,&readBytesRemovable);
		if (removalPointer && !(readBytesRemovable==inputChunkSize)){
			removalPointer = NULL;
		}
	}

	if (removalPointer){
		//printf("removalPointer \n"); fflush(stdout);
		// unpack, discard headers
		unpackPackets((unsigned char*) removalPointer, fftw_input_buffer, inputPacketCount);
		//printf("unpackPackets \n"); fflush(stdout);

		// FFT
		fftw_execute(plan);
		//printf("fftw_execute \n"); fflush(stdout);

		// compute stokes params
		stokes_xcorr(fftw_output_buffer,stokesBuffer);
		//printf("stokes_xcorr \n"); fflush(stdout);

		// integrate
		integrate(stokesBuffer,outputBuffer);
		//printf("integrate \n"); fflush(stdout);

		// write output
		bytesWritten = write(globals.opData.extFileHandle,outputBuffer,outputBufferSize);
		if (bytesWritten < 0){
			Log_Add("[OPERATION JUPITER]: Error writing data to output file\n");
		}
		//printf("write \n"); fflush(stdout);

		// free removed bytes in the queue
		RingQueue_NotifyRemovalComplete(&jupiterReadQueue,inputChunkSize);
		globals.opData.opFileCpyDmpCurrentPos += bytesWritten;
		removalPointer = NULL;
	}

	if (++rptcntr > 3000){
		rptcntr = 0;
		printf("Status ------------------------------------------------\n");
		printf("\tinputTotalBytesRead                        %lu\n",inputTotalBytesRead);
		printf("\toutputTotalBytesWritten                    %lu\n",globals.opData.opFileCpyDmpCurrentPos);
	}
	/*
		printf("readBytesToInsert                          %lu\n",readBytesToInsert);
		printf("readBytesInsertable                        %lu\n",readBytesInsertable);
		printf("readBytesToRemove                          %lu\n",readBytesToRemove);
		printf("readBytesRemovable                         %lu\n",readBytesRemovable);
		printf("bytesRead                                  %lu\n",bytesRead);
		printf("bytesWritten                               %lu\n",bytesWritten);
	*/
	if ((inputTotalBytesRead >= inputTotalBytesToRead) && (jupiterReadQueue.bytesUsed < inputChunkSize)){
		printf("Finished Jupiter\n");
		return SUCCESS;
	} else {
		return NOT_READY;
	}

}

StatusCode OpJupCleanup(){
	if (outputBuffer); free(outputBuffer); outputBuffer=NULL;
	if (stokesBuffer); free(stokesBuffer); stokesBuffer=NULL;
	if (fftw_output_buffer); fftw_free((void*) fftw_output_buffer); fftw_output_buffer=NULL;
	if (fftw_input_buffer); fftw_free((void*) fftw_input_buffer); fftw_input_buffer=NULL;
	if (jupiterReadQueue.data)
		RingQueue_Destroy(&jupiterReadQueue);
	if (globals.opData.tagfile->isOpen)
		FileSystem_CloseFile(globals.opData.tagfile);
	close(globals.opData.extFileHandle);
	Disk_Unmount(globals.opData.extdisk);

    readBytesToInsert	 = 0;
    readBytesInsertable	 = 0;
    readBytesToRemove	 = 0;
    readBytesRemovable	 = 0;
    bytesRead			 = 0;
    bytesWritten		 = 0;
    insertionPointer  	 = NULL;
    removalPointer 		 = NULL;
    globals.opData.opFileCpyDmpCurrentPos = 0;


return SUCCESS;
}
