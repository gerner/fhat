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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>

#include "util.h"

template <class T>
size_t sizeOfObject(T *obj);

size_t sizeOfObject(char *s) {
	return strlen(s)+1;
}

template <class T> 
size_t sizeOfObject(T *obj) {
	return sizeof(T);
}

template <class T>
int unpack(T *nodes, size_t nodeSize, FILE *input, FILE *output) {
	unsigned long long nodesRead=0;
	unsigned long long nodesWritten=0;

	size_t bufferCapacity = 1024;
	char *buffer = (char *)malloc(bufferCapacity);

	//for each edge
	u64 node;
	while(1 == fread(&node, sizeof(u64), 1, input)) {
		nodesRead++;
		//	write out translated edge to output
		fwrite(nodes+node, sizeOfObject(nodes+node), 1, output);
		nodesWritten++;
	}
	free(buffer);
	fprintf(stderr, "read %llu nodes, wrote %llu nodes\n", nodesRead, nodesWritten);
	return 0;
}

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "pack nodesfile input output");
		return -1;
	}
	assert(argc > 1);
	int nodesFd = open(argv[1], O_RDONLY);
	struct stat sb;
	int retval;
	retval = fstat(nodesFd, &sb);
	assert(0 == retval);
	
	FILE *input = stdin;
	if(argc > 2) {
		if(0 != strcmp(argv[2], "-")) {
			input = fopen(argv[2], "r");
		}
	}
	assert(input);
	FILE *output = stdout;
	if(argc > 3) {
		if(0 != strcmp(argv[3], "-")) {
			output = fopen(argv[3], "w");
		}
	}
	assert(output);

	assert(argc > 3);
	if(argc == 4 || 0 == strcmp(argv[4], "u64")) {	
		assert(sb.st_size % sizeof(unsigned long long) == 0);
		size_t numNodes = sb.st_size / sizeof(unsigned long long);
		//mmap nodes file
		unsigned long long *nodes = (unsigned long long*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, nodesFd, 0);
		unpack(nodes, numNodes, input, output);
		munmap(nodes, sb.st_size);
	} else if(0 == strcmp(argv[4], "string")) {
		size_t numNodes = sb.st_size;
		char *nodes = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, nodesFd, 0);
		unpack(nodes, numNodes, input, output);
		munmap(nodes, sb.st_size);
	}
	
	fclose(input);
	fclose(output);
	return 0;
}
