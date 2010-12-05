#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <map>
#include <string>
#include <vector>

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

const int T_CLASS = 2;	// Really an object
const int T_BOOLEAN = 4;
const int T_CHAR= 5;
const int T_FLOAT = 6;
const int T_DOUBLE = 7;
const int T_BYTE = 8;
const int T_SHORT = 9;
const int T_INT = 10;
const int T_LONG = 11;



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
	"unknown", // 0x0f
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

const char * HPROF_DUMP_RECORD_TYPES[] {
	"unknown",// 0x00
	"HPROF_GC_ROOT_JNI_GLOBAL",//    = 0x01;
	"HPROF_GC_ROOT_JNI_LOCAL",//     = 0x02;
	"HPROF_GC_ROOT_JAVA_FRAME",//    = 0x03;
	"HPROF_GC_ROOT_NATIVE_STACK",//  = 0x04;
	"HPROF_GC_ROOT_STICKY_CLASS",//  = 0x05;
	"HPROF_GC_ROOT_THREAD_BLOCK",//  = 0x06;
	"HPROF_GC_ROOT_MONITOR_USED",//  = 0x07;
	"HPROF_GC_ROOT_THREAD_OBJ",//    = 0x08;

	"unknown", // 0x09
	"unknown", // 0x0a
	"unknown", // 0x0b
	"unknown", // 0x0c
	"unknown", // 0x0d
	"unknown", // 0x0e
	"unknown", // 0x0f
	"unknown", // 0x10
	"unknown", // 0x11
	"unknown", // 0x12
	"unknown", // 0x13
	"unknown", // 0x14
	"unknown", // 0x15
	"unknown", // 0x16
	"unknown", // 0x17
	"unknown", // 0x18
	"unknown", // 0x19
	"unknown", // 0x1a
	"unknown", // 0x1b
	"HPROF_HEAP_DUMP_SEGMENT",//     = 0x1c;
	"unknown", // 0x1d
	"unknown", // 0x1e
	"unknown", // 0x1f

	"HPROF_GC_CLASS_DUMP",//         = 0x20;
	"HPROF_GC_INSTANCE_DUMP",//      = 0x21;
	"HPROF_GC_OBJ_ARRAY_DUMP",//         = 0x22;
	"HPROF_GC_PRIM_ARRAY_DUMP",//         = 0x23;

	NULL
};

const int MAX_DUMP_RECORD_TYPE = 0x24;

const char *getDumpRecordName(char recordType) {
	if(recordType < MAX_DUMP_RECORD_TYPE) {
		return HPROF_DUMP_RECORD_TYPES[(int)recordType];
	}
	else {
		return MAX_RECORD_NAME;
	}
}

//	"HPROF_GC_ROOT_UNKNOWN",//       = 0xff;
const size_t MAX_RECORD_SIZE_IN_MEMORY = 1024*1240*50;

size_t identifierSize = 0;
size_t positionInFile = 0;

int skipBytes(FILE *f, long offset) {
	positionInFile += offset;
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


size_t readBytes(char *v, size_t num, FILE *f) {
	int ret = fread(v, sizeof(char), num, f);
	assert(ret == (int)num);
	positionInFile += num;
	//assert(positionInFile == ftell(f));
	return ret;
}

long long int readLongLongInt(FILE *f) {
	long long int v;
		int ret = fread(&v, sizeof(long long int), 1, f);
		assert(ret);
		positionInFile += sizeof(long long int);
		//assert(positionInFile == ftell(f));
		return ntohll(v);
	}

	long int readIdentifier(FILE *f) {
		long int id;
		int ret = fread(&id, identifierSize, 1, f);
		assert(ret);
		positionInFile += identifierSize;
		//assert(positionInFile == ftell(f));
		if(4 == identifierSize) {
			return ntohl(id);
		}
		else {
			return ntohll(id);
		}
	}

	long int readIdentifier(char *record) {
		assert(4 == identifierSize || 8 == identifierSize);
		long int id;
		memcpy(&id, record, identifierSize);
		if(4 == identifierSize) {
			return ntohl(id);
		}
		else {
			return ntohll(id);
		}
	}

	int readInt(FILE *f) {
		int v;
		int ret = fread(&v, sizeof(int), 1, f);
		assert(ret);
		positionInFile += sizeof(int);
		//assert(positionInFile == ftell(f));
		return ntohl(v);
	}

	int readInt(char *record) {
		int v;
		memcpy(&v, record, sizeof(int));
		return ntohl(v);
	}

	unsigned short readUnsignedShort(FILE *f) {
		unsigned short v;
		int ret = fread(&v, sizeof(unsigned short), 1, f);
		assert(ret);
		positionInFile += sizeof(unsigned short);
		//assert(positionInFile == ftell(f));
		return ntohs(v);
	}

	char readByte(FILE *f) {
		char c;
		int ret = fread(&c, sizeof(char), 1, f);
		assert(ret);
		positionInFile += 1;
		//assert(positionInFile == ftell(f));
		return c;
	}

	char signatureFromTypeId(char typeId) {
		switch (typeId) {
		    case T_CLASS: {
			return 'L';
		    }
		    case T_BOOLEAN: {
			return 'Z';
		    }
		    case T_CHAR: {
			return 'C';
		    }
		    case T_FLOAT: {
			return 'F';
		    }
		    case T_DOUBLE: {
			return 'D';
		    }
		    case T_BYTE: {
			return 'B';
		    }
		    case T_SHORT: {
			return 'S';
		    }
		    case T_INT: {
			return 'I';
		    }
		    case T_LONG: {
			return 'J';
		    }
		    default: {
			fprintf(stderr, "unknown type of %c (%d)\n", typeId, (int)typeId);
			abort();
		    }
		}
	}

	size_t getJavaValueSize(char type) {
		switch (type) {
		    case '[': 
		    case 'L': {
			return identifierSize;
		    }
		    case 'Z': {
			return 1;
		    }
		    case 'B': {
			return 1;
		    }
		    case 'S': {
			return 2;
		    }
		    case 'C': {
			return 2;
		    }
		    case 'I': {
			return 4;
		    }
		    case 'J': {
			return 8;
		    }
		    case 'F': {
			return 4;
		    }
		    case 'D': {
			return 8;
		    }
		    default: {
			fprintf(stderr, "unknown java type '%c'", type);
			abort();
		    }
		}
	}
	
	struct StackFrame {
		long int id;
		int classSerialNumber;
		long int methodNameId;
		long int methodSigId;
		long int sourceFileId;
		int lineNumber;
		StackFrame() {
		}

		StackFrame(long int id, long int mn, long int ms, long int sf, int cln, int ln) {
			this->id = id;
			this->classSerialNumber = cln;
			this->methodNameId = mn;
			this->methodSigId = ms;
			this->sourceFileId = sf;
			this->lineNumber = ln;
		}
	};

	FILE *instancesFile;
	FILE *classesFile;
	FILE *referencesFile;
	
	std::map<long int, std::string> nameFromId;
	std::map<int, std::string> classNameFromSerialNumber;
	std::map<long int, std::string> classNameFromObjectId;
	std::map<long int, StackFrame> stackFrameFromId;
	std::map<long int, long int> superIdFromClassId;
	std::map<long int, std::vector<char> > fieldTypesFromClassId;
	std::map<long int, std::vector<std::string> > fieldNamesFromClassId;

	size_t readValue(FILE *input) {
		char type;
		int ret = fread(&type, sizeof(char), 1, input);
		assert(ret);
		positionInFile += 1;
		//assert(positionInFile == ftell(input));
		size_t vSize = getJavaValueSize(signatureFromTypeId(type));
		skipBytes(input, vSize);
		return vSize+1;
	}

	size_t readValueForType(char type, FILE *input) {
		    //fprintf(stderr, "reading value for type: %x\n", (int)type);
		size_t vSize = getJavaValueSize(signatureFromTypeId(type));
		skipBytes(input, vSize);
		return vSize;
	}

	int readArray(FILE *input, bool isPrimitive) {
		long int id = readIdentifier(input);
		int stackTraceSerial = readInt(input);
		//StackTrace stackTrace = getStackTraceFromSerial(stackTraceSerial);
		int num = readInt(input);
		int bytesRead = identifierSize + 8;
		long int elementClassID;
		if (isPrimitive) {
		    elementClassID = readByte(input);
		    bytesRead++;
		} else {
		    elementClassID = readIdentifier(input);
		    bytesRead += identifierSize;
		}
		
		// Check for primitive arrays:
		char primitiveSignature = 0x00;
		int elSize = 0;
		std::string primArrType = "";
		if (isPrimitive) {
		    switch (elementClassID) {
			case T_BOOLEAN: {
			    primitiveSignature = 'Z';
			    elSize = 1;
			    primArrType = "boolean[]";
			    break;
			}
			case T_CHAR: {
			    primitiveSignature = 'C';
			    elSize = 2;
			    primArrType = "char[]";
			    break;
			}
			case T_FLOAT: {
			    primitiveSignature = 'F';
			    elSize = 4;
			    primArrType = "float[]";
			    break;
			}
			case T_DOUBLE: {
			    primitiveSignature = 'D';
			    elSize = 8;
			    primArrType = "double[]";
			    break;
			}
			case T_BYTE: {
			    primitiveSignature = 'B';
			    elSize = 1;
			    primArrType = "byte[]";
			    break;
			}
			case T_SHORT: {
			    primitiveSignature = 'S';
			    elSize = 2;
			    primArrType = "short[]";
			    break;
			}
			case T_INT: {
			    primitiveSignature = 'I';
			    elSize = 4;
			    primArrType = "int[]";
			    break;
			}
			case T_LONG: {
			    primitiveSignature = 'J';
			    elSize = 8;
			    primArrType = "long[]";
			    break;
			}
		    }
		}
		else {
		    //fprintf(stderr, "ARRAY ELEMENT CLASS ID: %lu\n", elementClassID);
		    if(classNameFromObjectId.find(elementClassID) != classNameFromObjectId.end()) {
			    primArrType = "???";
		    }
		    else {
			    primArrType = classNameFromObjectId[elementClassID];
		    }
		}
		if (primitiveSignature != 0x00) {
		    //byte[] data = new byte[elSize * num];
		    skipBytes(input, elSize*num);
		    bytesRead += elSize*num;
		    //in.readFully(data);

		    /*if (version < VERSION_JDK12BETA4) {
			primArrType = null;	// They weren't named
		    }
		    JavaValueArray va 
			= new JavaValueArray(primitiveSignature, primArrType, 
					     data, stackTrace);
		    snapshot.addHeapObject(id, va);*/
		    fprintf (instancesFile, "array: %lu %lu %lu\n", id, elementClassID, (unsigned long)elSize*num);
		} else {
		    int arrayClassID = 0;
		    int sz = num * identifierSize;
		    bytesRead += sz;
		    //skipBytes(input, identifierSize * num);
		    fprintf(instancesFile, "array: %lu %lu %lu\n", id, elementClassID, identifierSize*num);
		    for (int i = 0; i < num; i++) {
			long int idElement = readIdentifier(input);
			fprintf(referencesFile, "reference: %lu %lu %lu []\n", id, elementClassID, idElement);
		    }
		    /*if (version >= VERSION_JDK12BETA4) {
			// It changed from the ID of the object describing the
			// class of element types to the ID of the object describing
			// the type of the array.
			arrayClassID = elementClassID;
			elementClassID = 0;
		    }
		    JavaObjectArray arr = 
			new JavaObjectArray(data, sz, stackTrace,
					    elementClassID, arrayClassID);
		    snapshot.addHeapObject(id, arr);*/
		}
		return bytesRead;
	    }




	int readClass(FILE *input) {
		size_t POS  = positionInFile;
		long int id = readIdentifier(input);
		assert(classNameFromObjectId.find(id) != classNameFromObjectId.end());
		int stackTraceId = readInt(input);
		long int superId = readIdentifier(input);
		superIdFromClassId[id] = superId;
		long int classLoaderId = readIdentifier(input);	// Ignored for now
		long int signersId = readIdentifier(input);	// Ignored for now
		long int protDomainId = readIdentifier(input);	// Ignored for now
		long int reserved1 = readIdentifier(input);
		long int reserved2 = readIdentifier(input);
		int instanceSize = readInt(input);
		int bytesRead = 7 * identifierSize + 8;

		int numConstPoolEntries = readUnsignedShort(input);
		bytesRead += 2;
		for (int i = 0; i < numConstPoolEntries; i++) {
		    int index = readUnsignedShort(input);	// unused
		    bytesRead += 2;
		    bytesRead += readValue(input);	// We ignore the values
		}

		int numStatics = readUnsignedShort(input);
		bytesRead += 2;
		std::vector<long int> staticReferences;
		std::vector<std::string> staticNames;
		for (int i = 0; i < numStatics; i++) {
		    long int nameId = readIdentifier(input);
		    assert(nameFromId.find(nameId) != nameFromId.end());
		    bytesRead += identifierSize;
		    char type = readByte(input);
		    bytesRead++;
		    if(T_CLASS == type) {
			    staticReferences.push_back(readIdentifier(input));
			    staticNames.push_back(nameFromId[nameId]);
			    bytesRead += identifierSize;
		    }
		    else {
			    bytesRead += readValueForType(type, input);
		    }
		}
		assert(positionInFile - POS == bytesRead);
		int numFields = readUnsignedShort(input);
		bytesRead += 2;
		fieldTypesFromClassId[id] = std::vector<char>();
		fieldNamesFromClassId[id] = std::vector<std::string>();
		for (int i = 0; i < numFields; i++) {
		    long int nameId = readIdentifier(input);
		    assert(nameFromId.find(nameId) != nameFromId.end());

		    bytesRead += identifierSize;
		    char type = readByte(input);
		    bytesRead++;
		    fieldTypesFromClassId[id].push_back(type);
		    fieldNamesFromClassId[id].push_back(nameFromId[nameId]);
		}

		fprintf(classesFile, "class: %lu %lu %s %d\n", id, superId, classNameFromObjectId[id].c_str(), bytesRead);
		for(int i=0; i<staticReferences.size();i++) {
			fprintf(referencesFile, "reference: %lu %lu %lu %s\n", id, id, staticReferences[i], staticNames[i].c_str());
		}

		return bytesRead;
	}

	size_t readInstance(FILE *input) {
		long int id = readIdentifier(input);
		int stackTraceId = readInt(input);
		long int classId = readIdentifier(input);
		size_t size = 4;
		assert(size >= 4);
		skipBytes(input, size-4);
		size += identifierSize*2 + 4;

		char b[4];
		readBytes(&b[0], 4, input);
		int b1 = b[0] & 0xff;
		int b2 = b[1] & 0xff;
		int b3 = b[2] & 0xff;
		int b4 = b[3] & 0xff;
		// Get the # of bytes for the field values:
		size_t bytesFollowing = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
		assert(classNameFromObjectId.find(classId) != classNameFromObjectId.end());
		fprintf(instancesFile, "instance: %lu %lu %lu\n", id, classId, classNameFromObjectId[classId].c_str(), bytesFollowing+size);
		//skipBytes(input, bytesFollowing);
		size_t bytesRead = 0;

		long int currentClassId = classId;
		while(currentClassId != 0) {
			assert(fieldTypesFromClassId.find(currentClassId) != fieldTypesFromClassId.end());
			assert(fieldNamesFromClassId.find(currentClassId) != fieldNamesFromClassId.end());
			std::vector<char> fields = fieldTypesFromClassId[currentClassId];
			std::vector<std::string> fieldNames = fieldNamesFromClassId[currentClassId];
			for(int i=0; i<fields.size(); i++) {
				if(T_CLASS == fields[i]) {                                     	
                                        long int referenceId = readIdentifier(input);
					fprintf(referencesFile, "reference: %lu %lu %lu %s\n", id, currentClassId, referenceId, fieldNames[i].c_str());
					bytesRead += identifierSize;
                                }
                                else {
                                        bytesRead += readValueForType(fields[i], input);
                                }
			}
			assert(superIdFromClassId.find(currentClassId) != superIdFromClassId.end());
			currentClassId = superIdFromClassId[currentClassId];
		}
		assert(bytesFollowing == bytesRead);

		return size + bytesFollowing;
	}
	int readHeapDump(FILE *input, size_t dumpSize) {
		int ret;
		fprintf(stderr, "heap dump of size %lu bytes\n", dumpSize);
		

		size_t dumpStartPosition = positionInFile;
		size_t bytesLeft = dumpSize;
		char recordType;
		unsigned long long records = 0;
		while(positionInFile - dumpStartPosition < dumpSize) {
			//assert(positionInFile == ftell(input));
			//assert(positionInFile - dumpStartPosition < dumpSize);
			assert((dumpSize - bytesLeft) == (positionInFile - dumpStartPosition));
			recordType = readByte(input);
			bytesLeft--;
			records++;
			switch(recordType) {
			/*case HPROF_GC_ROOT_UNKNOWN: {
			    long int id = readIdentifier(input);
			    bytesLeft -= identifierSize;
			    snapshot.addRoot(new Root(id, 0, Root.UNKNOWN, ""));
			    break;
			}*/
			case HPROF_GC_ROOT_THREAD_OBJ: {
			    long int id = readIdentifier(input);
			    int threadSeq = readInt(input);
			    int stackSeq = readInt(input);
			    bytesLeft -= identifierSize + 8;
			    /*threadObjects.put(new Integer(threadSeq), new ThreadObject(id, stackSeq));*/
			    break;
			}
			case HPROF_GC_ROOT_JNI_GLOBAL: {
			    long int id = readIdentifier(input);
			    int globalRefId = readIdentifier(input);	// Ignored, for now
			    bytesLeft -= 2*identifierSize;
			    /*snapshot.addRoot(new Root(id, 0, Root.NATIVE_STATIC, ""));*/
			    break;
			}
			case HPROF_GC_ROOT_JNI_LOCAL: {
			    long int id = readIdentifier(input);
			    int threadSeq = readInt(input);
			    int depth = readInt(input);
			    bytesLeft -= identifierSize + 8;
			    /*ThreadObject to = getThreadObjectFromSequence(threadSeq);
			    StackTrace st = getStackTraceFromSerial(to.stackSeq);
			    if (st != null) {
				st = st.traceForDepth(depth+1);
			    }
			    snapshot.addRoot(new Root(id, to.threadId, Root.NATIVE_LOCAL, "", st));*/
			    break;
			}
			case HPROF_GC_ROOT_JAVA_FRAME: {
			    long int id = readIdentifier(input);
			    int threadSeq = readInt(input);
			    int depth = readInt(input);
			    bytesLeft -= identifierSize + 8;
			    /*ThreadObject to = getThreadObjectFromSequence(threadSeq);
			    StackTrace st = getStackTraceFromSerial(to.stackSeq);
			    if (st != null) {
				st = st.traceForDepth(depth+1);
			    }
			    snapshot.addRoot(new Root(id, to.threadId, Root.JAVA_LOCAL, "", st));*/
			    break;
			}
			case HPROF_GC_ROOT_NATIVE_STACK: {
			    long int id = readIdentifier(input);
			    int threadSeq = readInt(input);
			    bytesLeft -= identifierSize + 4;
			    /*ThreadObject to = getThreadObjectFromSequence(threadSeq);
			    StackTrace st = getStackTraceFromSerial(to.stackSeq);
			    snapshot.addRoot(new Root(id, to.threadId, Root.NATIVE_STACK, "", st));*/
			    break;
			}
			case HPROF_GC_ROOT_STICKY_CLASS: {
			    long int id = readIdentifier(input);
			    bytesLeft -= identifierSize;
			    /*snapshot.addRoot(new Root(id, 0, Root.SYSTEM_CLASS, ""));*/
			    break;
			}
			case HPROF_GC_ROOT_THREAD_BLOCK: {
			    long int id = readIdentifier(input);
			    int threadSeq = readInt(input);
			    bytesLeft -= identifierSize + 4;
			    /*ThreadObject to = getThreadObjectFromSequence(threadSeq);
			    StackTrace st = getStackTraceFromSerial(to.stackSeq);
			    snapshot.addRoot(new Root(id, to.threadId, 
					     Root.THREAD_BLOCK, "", st));*/
			    break;
			}
			case HPROF_GC_ROOT_MONITOR_USED: {
			    long int id = readIdentifier(input);
			    bytesLeft -= identifierSize;
			    /*snapshot.addRoot(new Root(id, 0, Root.BUSY_MONITOR, ""));*/
			    break;
			}
			case HPROF_GC_CLASS_DUMP: {
			    int bytesRead = readClass(input);
			    bytesLeft -= bytesRead;
			    break;
			}
			case HPROF_GC_INSTANCE_DUMP: {
			    bytesLeft -= readInstance(input);
			    break;
			}
			case HPROF_GC_OBJ_ARRAY_DUMP: {
			    int bytesRead = readArray(input, false);
			    bytesLeft -= bytesRead;
			    break;
			}
			case HPROF_GC_PRIM_ARRAY_DUMP: {
			    int bytesRead = readArray(input, true);
			    bytesLeft -= bytesRead;
			    break;
			}
			default: {
				fprintf(stderr, "unknown record type! %d\n", (int)recordType);
				abort(); 
			}
		    }	
		}
		assert(positionInFile - dumpStartPosition == dumpSize);
		return 0;
	}

	int main(int argc, char **argv) {
		assert(argc > 4);
		FILE *input;
		if(strcmp(argv[1], "-") == 0) {
			input = stdin;
		}
		else {
			input = fopen(argv[1], "r");
		}
		if(strcmp(argv[2], "-") == 0) {
			classesFile = stdout;
		}
		else {
			classesFile = fopen(argv[2], "w");
		}
		if(strcmp(argv[3], "-") == 0) {
			instancesFile = stdout;
		}
		else {
			instancesFile = fopen(argv[3], "w");
		}
		if(strcmp(argv[4], "-") == 0) {
			referencesFile = stdout;
		}
		else {
			referencesFile = fopen(argv[4], "w");
		}


		int ret;

		//read the header
		long long int creationTimestamp;
		char *versionBuffer = NULL;
		size_t versionBufferSize = 0;
		size_t vbs = getdelim(&versionBuffer, &versionBufferSize, 0, input);
		assert(4 < vbs);
		fprintf(stderr, "version is: \"%s\" %lu\n", versionBuffer, vbs);
		positionInFile += vbs;

		identifierSize = readInt(input);
		creationTimestamp = readLongLongInt(input);
		char *creationTimeString = ctime((const time_t *)&creationTimestamp);
		creationTimeString[strlen(creationTimeString)-1]=0;

		fprintf(stderr, "identifiers are %lu bytes\n", identifierSize);
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
			positionInFile += 1;
			//assert(positionInFile == ftell(input));
			recordTimestamp = readInt(input);
			recordLength = readInt(input);
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
				ret = skipBytes(input, recordLength);
				assert(!ret);
			}
			else if(recordType == HPROF_HEAP_DUMP_SEGMENT || recordType == HPROF_HEAP_DUMP) {
				readHeapDump(input, recordLength);
			}
			else {
				assert(MAX_RECORD_SIZE_IN_MEMORY >= recordLength);

				assert(recordBufferSize > 0);
				if(recordBufferSize < recordLength+1) {
					fprintf(stderr, "record buffer not big enough (%lu), growing\n", recordBufferSize);
					recordBufferSize = std::max(recordBufferSize*2, recordLength+1);
					record = (char *)realloc(record, recordBufferSize);
				}
				
				readBytes(record, recordLength, input);

				long int id;
				int serialNumber;
				long int classId;
				int stackTraceSerialNumber;
				long int classNameId;
				switch(recordType) {
					case HPROF_UTF8 :
						assert(recordLength >= identifierSize);
						id = readIdentifier(record);
						record[recordLength]=0;
						nameFromId[id] = std::string(record+identifierSize);
						break;
					case HPROF_LOAD_CLASS :
						assert(recordLength == sizeof(int)*2 + identifierSize*2);
						serialNumber = readInt(record);
						classId = readIdentifier(record+sizeof(int));
						stackTraceSerialNumber = readInt(record+sizeof(int)+identifierSize);
						classNameId = readIdentifier(record+sizeof(int)*2+identifierSize);
						assert(nameFromId.find(classNameId) != nameFromId.end());
						classNameFromObjectId[classId] = nameFromId[classNameId];
						classNameFromSerialNumber[serialNumber] = nameFromId[classNameId];
						break;
					case HPROF_FRAME :
						assert(recordLength == identifierSize * 4 + sizeof(int)*2);
						id = readIdentifier(record);
						stackFrameFromId[id] = StackFrame(
								id,
								readIdentifier(record),
								readIdentifier(record),
								readIdentifier(record),
								readInt(record),
								readInt(record));
						break;
					//default :
						//no op
				}
			}

			//return 0;
		}
		fprintf(stderr, "%llu records done.\n", numRecords);
		fprintf(stderr, "%lu names\n", nameFromId.size());
		fprintf(stderr, "%lu classes\n", classNameFromObjectId.size());
		fprintf(stderr, "%lu frames\n", stackFrameFromId.size());

		fclose(classesFile);
		fclose(instancesFile);
		fclose(referencesFile);
	}


