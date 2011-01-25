#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <string>
#include <queue>
#include <unistd.h>

#include "util.h"

#define SORT_BUFFER_SIZE 1024*1024*500

template <class T>
void copyRecord(T *dst, T *src) {
	memcpy(dst, src, sizeof(T));
}

template <class T>
struct SortedFile
{
	FILE *f;
	T *next;

	SortedFile(const char *fname) {
		this->f = fopen(fname, "r");
		this->next = new T;
	}

	~SortedFile() {
		fclose(this->f);
		delete this->next;
	}

	bool readNext() {
		return 1 == readRecord(this->next, this->f);
	}
};

template <class T, class Compare>
struct CompareSortedFiles {
	Compare cmp;
	CompareSortedFiles(Compare cmp = Compare()) {
		this->cmp = cmp;
	}
	bool operator()(const SortedFile<T> *lhs, const SortedFile<T> *rhs) const {
		//we want to use this with a priority queue, so we swap the relationship so that the smallest item appears the biggest
		return cmp(*(rhs->next), *(lhs->next));
	}
};

template <class T, class Compare>
int sortFile(FILE *in, FILE *out, bool unique, Compare cmp = Compare()) {
	assert(in);
	assert(out);
	std::vector<std::string> tempFiles;

	T *buffer = new T[SORT_BUFFER_SIZE/sizeof(T)];

	unsigned long long itemsRead = 0;
	unsigned long long itemsWrittenWithDups = 0;
	unsigned long long itemsWritten = 0;
	bool foundEOF = false;

	//create sorted runs
	//while more to read
	while(!foundEOF) {
		//read from in until we fill a buffer
		T *bufPtr = buffer;
		while(bufPtr + 1 < buffer + (SORT_BUFFER_SIZE/sizeof(T))) {
			if(1 != readRecord(bufPtr, in)) {
				assert(feof(in));
				foundEOF = true;
				fprintf(stderr, "input closed\n");
				break;
			}
			itemsRead++;
			bufPtr++;
		}

		//sort it in memory
		std::sort(buffer, bufPtr, cmp);

		//short circuit if the whole thing fit in memory
		if(foundEOF && tempFiles.size() == 0) {
			fprintf(stderr, "input fit in memory, short circuit\n");
			T *ptr = buffer;
			while(ptr < bufPtr) {
				if(!unique || ptr == buffer || cmp(*(ptr-1), *ptr)) {
					int retval = writeRecord(ptr, out);
					assert(1 == retval);
					itemsWritten++;
				}
				assert(ptr == buffer || !(cmp(*(ptr), *(ptr-1))));
				ptr++;
				itemsWrittenWithDups++;
			}
			assert(itemsRead == itemsWrittenWithDups);
			fprintf(stderr, "%llu records read, %llu records written\n", itemsRead, itemsWritten);
			return 0;
		}

		//write it to a tempfile
		char tempFileName[32];
		strcpy(tempFileName, "/tmp/sortFileXXXXXX");
		//make a temp file
		FILE *tempStream = fdopen(mkstemp(tempFileName), "w");
		T *ptr = buffer;
		while (ptr < bufPtr) {
			if(ptr == buffer || !unique || (ptr > buffer && cmp(*(ptr-1), *ptr))) {
				int retval = writeRecord(ptr, tempStream);
				assert(1 == retval);
				itemsWritten++;
			}
			assert(ptr == buffer || !(cmp(*(ptr), *(ptr-1))));
			ptr++;
			itemsWrittenWithDups++;
		}
		fclose(tempStream);
		tempFiles.push_back(std::string(tempFileName));
	}
	assert(tempFiles.size() > 0);
	assert(itemsWrittenWithDups == itemsRead);
	delete [] buffer;

	itemsWrittenWithDups = 0;
	unsigned long long itemsWrittenToRuns = itemsWritten;
	itemsWritten = 0;
	unsigned long long tempItemsRead = 0;
	//merge sorted runs:
	fprintf(stderr, "merging %lu sorted runs of (%llu records total)\n", tempFiles.size(), itemsWrittenToRuns);
	//open all temp files and populate SortedFile instances for each
	std::priority_queue<SortedFile<T> *, std::vector<SortedFile<T> *>, CompareSortedFiles<T, Compare> > runs;
	for(unsigned int i=0; i<tempFiles.size(); i++) {
		SortedFile<T> *s = new SortedFile<T>(tempFiles[i].c_str());
		//read next from each and place into a pq (if next)
		if(s->readNext()) {
			runs.push(s);
			tempItemsRead++;
		}
	}

	T *last = NULL;

	//while pq not empty
	while(!runs.empty()) {
		//	pop from pq
		SortedFile<T> *s = runs.top();
		assert(s);
		assert(s->next);
		runs.pop();
		//	write to output
		assert(NULL == last || !(cmp(*(s->next), *last)));

		if(!unique || NULL == last || cmp(*last, *(s->next))) {
			writeRecord(s->next, out);
			itemsWritten++;
			if(unique) {
				if(NULL == last) {
					last = new T;
				}
				copyRecord(last, s->next);
			}
		}
		itemsWrittenWithDups++;
		//  if readNext, place back on queue
		if(s->readNext()) {
			runs.push(s);
			tempItemsRead++;
		} else {
			//fprintf(stderr, "finished a run\n");
			delete s;
		}
	}
	//nuke all the temp files
	for(unsigned int i=0; i<tempFiles.size(); i++) {
		remove(tempFiles[i].c_str());
	}
	assert(itemsWrittenToRuns == tempItemsRead);
	assert(itemsWrittenToRuns == itemsWrittenWithDups);
	delete last;

	fprintf(stderr, "%llu records read, %llu records merged, %llu records written\n", itemsRead, tempItemsRead, itemsWritten);

	return 0;	
}
FILE *input;
FILE *output;

int main(int argc, char **argv)
{
	char type[32];
	if(argc < 3) {
		input = stdin;
		output = stdout;
		strcpy(type, "link");
	} else {
		assert(argc == 4);
		if(0 == strcmp(argv[1], "-")) {
			input = stdin;
		} else {
			input = fopen(argv[1], "r");
		}
		if(0 == strcmp(argv[2], "-")) {
			output = stdout;
		} else {
			output = fopen(argv[2], "w");
		}
		assert(strlen(argv[3]) < 31);
		strcpy(type, argv[3]);
	}

	if(0 == strcmp(type, "link")) {
		sortFile<Link, std::less<Link> >(input, output, true);
	} else if(0 == strcmp(type, "u64")) {
		sortFile<u64, std::less<u64> >(input, output, true);
	} else if(0 == strcmp(type, "u64_3")) {
		sortFile<U64_N<3>, std::less<U64_N<3> > >(input, output, true);
	} else if(0 == strcmp(type, "ru64_4")) {
		sortFile<U64_N<4>, std::greater<U64_N<4> > >(input, output, true);
	} else if(0 == strcmp(type, "ru64_5")) {
		sortFile<U64_N<5>, std::greater<U64_N<5> > >(input, output, true);
	} else {
		abort();
	}
	fclose(input);
	fclose(output);
	return 0;
}
