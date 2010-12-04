#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(x >> 32)) ) )
#define htonll(x) ntohll(x)

//
// Record types:
//
const int HPROF_UTF8          = 0x01;
const int HPROF_LOAD_CLASS    = 0x02;
const int HPROF_UNLOAD_CLASS  = 0x03;
const int HPROF_FRAME         = 0x04;
const int HPROF_TRACE         = 0x05;
const int HPROF_ALLOC_SITES   = 0x06;
const int HPROF_HEAP_SUMMARY  = 0x07;

const int HPROF_START_THREAD  = 0x0a;
const int HPROF_END_THREAD    = 0x0b;

const int HPROF_HEAP_DUMP     = 0x0c;

const int HPROF_CPU_SAMPLES   = 0x0d;
const int HPROF_CONTROL_SETTINGS = 0x0e;
const int HPROF_LOCKSTATS_WAIT_TIME = 0x10;
const int HPROF_LOCKSTATS_HOLD_TIME = 0x11;

const int HPROF_GC_ROOT_UNKNOWN       = 0xff;
const int HPROF_GC_ROOT_JNI_GLOBAL    = 0x01;
const int HPROF_GC_ROOT_JNI_LOCAL     = 0x02;
const int HPROF_GC_ROOT_JAVA_FRAME    = 0x03;
const int HPROF_GC_ROOT_NATIVE_STACK  = 0x04;
const int HPROF_GC_ROOT_STICKY_CLASS  = 0x05;
const int HPROF_GC_ROOT_THREAD_BLOCK  = 0x06;
const int HPROF_GC_ROOT_MONITOR_USED  = 0x07;
const int HPROF_GC_ROOT_THREAD_OBJ    = 0x08;

const int HPROF_GC_CLASS_DUMP         = 0x20;
const int HPROF_GC_INSTANCE_DUMP      = 0x21;
const int HPROF_GC_OBJ_ARRAY_DUMP         = 0x22;
const int HPROF_GC_PRIM_ARRAY_DUMP         = 0x23;

const int HPROF_HEAP_DUMP_SEGMENT     = 0x1c;
const int HPROF_HEAP_DUMP_END         = 0x2c;

const char * HPROF_RECORD_TYPES[] {
	"unknown",
	"HPROF_UTF8",//          = 0x01;
	"HPROF_LOAD_CLASS",//    = 0x02;
	"HPROF_UNLOAD_CLASS",//  = 0x03;
	"HPROF_FRAME",//         = 0x04;
	"HPROF_TRACE",//         = 0x05;
	"HPROF_ALLOC_SITES",//   = 0x06;
	"HPROF_HEAP_SUMMARY",//  = 0x07;
	
	"unknown", // 0x08
	"unknown", // 0x09

	"HPROF_START_THREAD",//  = 0x0a;
	"HPROF_END_THREAD",//    = 0x0b;

	"HPROF_HEAP_DUMP",//     = 0x0c;

	"HPROF_CPU_SAMPLES",//   = 0x0d;
	"HPROF_CONTROL_SETTINGS",// = 0x0e;
	"HPROF_LOCKSTATS_WAIT_TIME",// = 0x10;
	"HPROF_LOCKSTATS_HOLD_TIME",// = 0x11;
	NULL
};
const char MAX_RECORD_TYPE = 0x12;
const char *MAX_RECORD_NAME = "over max";

const char *getRecordName(char recordType) {
	if(recordType < MAX_RECORD_TYPE) {
		return HPROF_RECORD_TYPES[(int)recordType];
	}
	else {
		return MAX_RECORD_NAME;
	}
}

/*
	"HPROF_GC_ROOT_UNKNOWN",//       = 0xff;
	"HPROF_GC_ROOT_JNI_GLOBAL",//    = 0x01;
	"HPROF_GC_ROOT_JNI_LOCAL",//     = 0x02;
	"HPROF_GC_ROOT_JAVA_FRAME",//    = 0x03;
	"HPROF_GC_ROOT_NATIVE_STACK",//  = 0x04;
	"HPROF_GC_ROOT_STICKY_CLASS",//  = 0x05;
	"HPROF_GC_ROOT_THREAD_BLOCK",//  = 0x06;
	"HPROF_GC_ROOT_MONITOR_USED",//  = 0x07;
	"HPROF_GC_ROOT_THREAD_OBJ",//    = 0x08;

	"HPROF_GC_CLASS_DUMP",//         = 0x20;
	"HPROF_GC_INSTANCE_DUMP",//      = 0x21;
	"HPROF_GC_OBJ_ARRAY_DUMP",//         = 0x22;
	"HPROF_GC_PRIM_ARRAY_DUMP",//         = 0x23;

	"HPROF_HEAP_DUMP_SEGMENT",//     = 0x1c;
	"HPROF_HEAP_DUMP_END",//         = 0x2c;


}*/

const size_t MAX_RECORD_SIZE_IN_MEMORY = 1024*1240*50;

int skipBytes(FILE *f, long offset) {
	if(f != stdin) {
		return fseek(f, offset, SEEK_CUR);
	}
	else {
		for(;offset>0;offset--) {
			int c = fgetc(f);
			if(c == EOF) {
				return -1;
			}
		}
		return 0;
	}
}

int main(int argc, char **argv) {
	assert(argc > 1);
	FILE *input;
	if(strcmp(argv[1], "-") == 0) {
		input = stdin;
	}
	else {
		input = fopen(argv[1], "r");
	}


	int ret;

	//read the header
	int identifierSize;
	long long int creationTimestamp;
	char *versionBuffer = NULL;
	size_t versionBufferSize = 0;
	size_t vbs = getdelim(&versionBuffer, &versionBufferSize, 0, input);
	assert(4 < vbs);
	fprintf(stderr, "version is: \"%s\" %lu\n", versionBuffer, vbs);

	//for some reason we need to skip another 4 bytes...
	ret = fread(&identifierSize, sizeof(int), 1, input);
	assert(ret);
	identifierSize = ntohl(identifierSize);
	//let's not support 32 bits :)
	assert(identifierSize == 8);
	ret = fread(&creationTimestamp, sizeof(long long int), 1, input);
	assert(ret);
	creationTimestamp = ntohll(creationTimestamp)/1000;
	char *creationTimeString = ctime((const time_t *)&creationTimestamp);
	creationTimeString[strlen(creationTimeString)-1]=0;

	fprintf(stderr, "identifiers are %d bytes long\n", identifierSize);
	fprintf(stderr, "created on %s (%lld)\n", creationTimeString, creationTimestamp);

	size_t recordBufferSize = 1024;
	char *record = (char *)malloc(recordBufferSize);

	char recordType;
	int recordTimestamp;
	size_t recordLength;

	unsigned long long numRecords = 0;
		
	while(true) {
		if(feof(input)) {
			break;
		}

		ret = fread(&recordType, sizeof(char), 1, input);
		if(!ret) {
			assert(feof(input));
			break;
		}
		ret = fread(&recordTimestamp, sizeof(int), 1, input);
		assert(ret);
		recordTimestamp = ntohl(recordTimestamp);
		ret = fread(&recordLength, sizeof(int), 1, input);
		assert(ret);
		recordLength = ntohl(recordLength);
		fprintf(stdout, "%llu %d: %d (%s) of length %lu\n", numRecords, recordTimestamp, (int)recordType, getRecordName(recordType), recordLength);
		numRecords++;


		if(recordType == HPROF_UNLOAD_CLASS 
			|| recordType == HPROF_ALLOC_SITES
			|| recordType == HPROF_START_THREAD
			|| recordType == HPROF_END_THREAD
			|| recordType == HPROF_HEAP_SUMMARY
			|| recordType == HPROF_CPU_SAMPLES
			|| recordType == HPROF_CONTROL_SETTINGS
			|| recordType == HPROF_LOCKSTATS_WAIT_TIME
			|| recordType == HPROF_LOCKSTATS_HOLD_TIME)
		{
			//we don't care about these records
			fprintf(stdout, "\tskipping record\n");
			ret = skipBytes(input, recordLength);
			assert(!ret);
		}
		else if(recordType == HPROF_HEAP_DUMP_SEGMENT || recordType == HPROF_HEAP_DUMP) {
			fprintf(stdout, "\tskipping heap dump\n");
			ret = skipBytes(input, recordLength);
			fprintf(stderr, "error: %d %s\n", errno, strerror(errno));
			assert(!ret);
		}
		else {
			assert(MAX_RECORD_SIZE_IN_MEMORY >= recordLength);

			assert(recordBufferSize > 0);
			if(recordBufferSize < recordLength+1) {
				fprintf(stderr, "record buffer not big enough (%lu), growing\n", recordBufferSize);
				record = (char *)realloc(record, recordLength+1);
				recordBufferSize = recordLength+1;
			}
			
			ret = fread(record, recordLength, 1, input);
			assert(ret);

			switch(recordType) {
				case HPROF_UTF8 :
					long int id;
					memcpy(&id, record, sizeof(long int));
					id = ntohll(id);
					record[recordLength]=0;
					fprintf(stdout, "\t%ld -> %s\n", id, record+identifierSize);
					break;
				//default :
					//no op
			}
		}

		//return 0;
	}
	fprintf(stderr, "%llu records done.\n", numRecords);
}

