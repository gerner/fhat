#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>
#include <string.h>
#include "util.h"

template <class T, class V>
u64 sortColumn(T *sortedKey, V *sortedValue, size_t numRecords, FILE *unsortedKey, FILE *unsortedValue) {
	T *k = (T *)malloc(sizeof(T));
	V *v = (V *)malloc(sizeof(V));

	u64 recordsSorted = 0;
	while(1 == readRecord(k, unsortedKey)) {
		int retval = readRecord(v, unsortedValue);
		assert(1 == retval);
		T *ptr = std::lower_bound(sortedKey, sortedKey+numRecords, *k);
		assert(ptr >= sortedKey);
		assert(ptr < sortedKey+numRecords);
		assert(*ptr == *k);
		sortedValue[ptr - sortedKey] = *v;
		recordsSorted++;
	}

	free(k);
	free(v);

	return recordsSorted;
}

template <class T, class V>
u64 prepareAndSortColumn(int keyFd, int valueFd, FILE *unsortedKey, FILE *unsortedValue) {
	assert(keyFd >= 0);
	assert(valueFd >= 0);
	assert(unsortedKey);
	assert(unsortedValue);

	struct stat sb;
	int retval = fstat(keyFd, &sb);
	assert(sb.st_size % sizeof(T) == 0);
	unsigned long long numRecords = sb.st_size/sizeof(T);

	T *keys;
	V *values;
	initializeFile(valueFd, numRecords*sizeof(V));
	
	//mmap keys and values
	keys = (T *)mmap(NULL, numRecords*sizeof(T), PROT_READ, MAP_SHARED, keyFd, 0);
	assert(MAP_FAILED != keys);
	values = (V *)mmap(NULL, numRecords*sizeof(V), PROT_READ | PROT_WRITE, MAP_SHARED, valueFd, 0);
	assert(MAP_FAILED != values);
	
	//sort the column
	u64 sortRetval = sortColumn(keys, values, numRecords, unsortedKey, unsortedValue);	

	//munmap keys and values
	retval = munmap(keys, numRecords*sizeof(T));
	assert(retval != -1);
	retval = munmap(values, numRecords*sizeof(V));
	assert(retval != -1);

	return sortRetval;
}

int main(int argc, char **argv) {
	if(argc < 7) {
		fprintf(stderr, "sortcolumns key_type unsorted_key col_type unsorted_col sorted_key sorted_col\n");
		return 0;
	}
	//input:
	//	unsorted key column
	//	sorted key column
	//	value column
	//output:
	//	value column sorted
	char *keyType = argv[1];
	char *valueType = argv[3];
	
	FILE *unsortedKeyFile = fopen(argv[2], "r");
	assert(unsortedKeyFile);
	FILE *unsortedValueFile = fopen(argv[4], "r");
	assert(unsortedValueFile);
		
	int sortedKeyFd = open(argv[5], O_RDONLY);
	assert(sortedKeyFd >= 0);
	int sortedValueFd = open(argv[6], O_CREAT | O_TRUNC | O_RDWR, 0600);
	assert(sortedValueFd >= 0);

	//get streams for unsorted key column and all unsorted value columns
	//for each unsorted key
	//	binary search sorted key column to find insertion location
	//	read a value from unsorted file
	//	insert into right location
	u64 recordsSorted;
	if(0 == strcmp(valueType, "u64")) {
		if(0 == strcmp(keyType, "u64")) {
			recordsSorted = prepareAndSortColumn<u64, u64>(sortedKeyFd, sortedValueFd, unsortedKeyFile, unsortedValueFile);
		} else {
			abort();
		}
	} else if(0 == strcmp(valueType, "link")) {
		if(0 == strcmp(keyType, "u64")) {
			recordsSorted = prepareAndSortColumn<u64, Link>(sortedKeyFd, sortedValueFd, unsortedKeyFile, unsortedValueFile);
		} else {
			abort();
		}
	} else {
		abort();
	}
	fprintf(stderr, "sorted %llu records from %s\n", recordsSorted, argv[4]); 

	fclose(unsortedKeyFile);
	fclose(unsortedValueFile);
	close(sortedKeyFd);
	close(sortedValueFd);
	return 0;
}
