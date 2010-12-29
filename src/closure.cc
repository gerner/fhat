#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <string>

#define SORT_BUFFER_SIZE 1024*1024*100

struct LinkByTarget {
	long long int src;
	long long int tgt;
};

bool operator<(const LinkByTarget &lhs, const LinkByTarget &rhs) {
	if (lhs.src < rhs.src) {
		return true;
	}
	else if (lhs.src == rhs.src) {
		return lhs.tgt < rhs.tgt;
	}
	return false;
}

template <class T>
int sortFile(FILE *in, FILE *out) {
	std::vector<std::string> tempFiles;

	T *buffer = (T*)malloc(SORT_BUFFER_SIZE);

	unsigned long long val;
	bool foundEOF = false;

	//while more to read
	while(!foundEOF) {
		//read from in until we fill a buffer
		size_t currentSize = 0;
		while(sizeof(T) + currentSize < SORT_BUFFER_SIZE) {
			if(fread((char *)buffer + currentSize, sizeof(T), 1, in) != 1) {
				assert(feof(in));
				foundEOF = true;
				break;
			}
			currentSize += sizeof(T);
		}

		//sort it in memory
		std::sort<T>(buffer, buffer+currentSize);

		//short circuit if the whole thing fit in memory
		if(foundEOF && tempFiles.size() == 0) {
			T *ptr = buffer;
			while(ptr < buffer + currentSize) {
				int retval = fwrite(ptr, sizeof(T), 1, out);
				assert(1 == retval);
				ptr++;
			}
			return;
		}

		//write it to a tempfile
		char tempFileName[32];
		strcpy(tempFileName, "sortFileXXXXXX");
		//make a temp file
		FILE *tempStream = fdopen(mkstemp(tempFileName), "w");
		T *ptr = buffer;
		while (ptr < buffer + currentSize) {
			int retval = fwrite(ptr, sizeof(T), 1, out);
			assert(1 == retval);
			ptr++;
		}
		fclose(tempStream);
		tempFiles.push_back(std::string(tempFileName));
	}

	free(buffer);

	//open each tempfile
	//pass over them and merge the results to out
	//nuke the tempfiles
}

int main(int argc, char **argv) {
	//assume we've got a file = {(src,tgt)} sorted (src,tgt)
	
	char *nameC = (char *)malloc(sizeof(char)*32);
	strcpy(nameC, "cXXXXXX");
	char *nameT = (char *)malloc(sizeof(char)*32);
	strcpy(nameT, "tXXXXXX");

	//initialize:
	//	create a new empty file C (this will be the transitive closure)
	//int fdC = mkstemp(nameC);
	//FILE *fileC = fdopen(fdC, "w");

	//	create a new file T equal to R sorted by (tgt,src)
	//int fdT = mkstemp(nameT);
	
	//invariants:
	//	T is the latest set of discovered transitive links
	//	C is the set of all paths of length <= k (k is the iteration)
	
	//in K iterations (where K is the longest path)
	//while T is not empty:
	//	temp file T'
	//
	//	in O(n)
	//	pass over each (s, t') from T, (t', t) from R
	//		assert (s, t) bigger that last (s, t) (by tgt,src)
	//		output (s, t) to T'
	//		
	//	in O(n log n)
	//	create T'' by sorting T' (src,tgt)
	//
	//	in O(n)
	//	temp file C'
	//	temp file T'''
	//	pass over T'' and C
	//		output (s, t) to C' if (s, t) in either T'' or C
	//		output (s, t) to  T''' if (s, t) in T'' and not C
	//
	//	in O(n log n)
	//	sort T''' by (tgt, src) and rename to T
	//	rename C' to C
	
	//C is the set {(s, t)} of objects t reachable by a path from s

	return 0;
}
