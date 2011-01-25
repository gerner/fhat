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

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "pack nodesfile input output [orphan_flags]");
		fprintf(stderr, "\torphan_flags : 0 => abort, 1=>turn to zero\n");
		return -1;
	}
	assert(argc > 1);
	int nodesFd = open(argv[1], O_RDONLY);
	struct stat sb;
	int retval;
	retval = fstat(nodesFd, &sb);
	assert(sb.st_size % sizeof(unsigned long long) == 0);
	size_t numNodes = sb.st_size / sizeof(unsigned long long);
	assert(0 == retval);
	//mmap nodes file
	unsigned long long *nodes = (unsigned long long*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, nodesFd, 0);
	
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
	
	FILE *orphansFile = NULL;
	if(argc > 4 &&  0 == strcmp(argv[4], "1")) {
		orphansFile = fopen("packorphans", "w");
	}

	unsigned long long orphans=0;
	unsigned long long nodesRead=0;
	unsigned long long nodesWritten=0;

	//for each edge
	u64 node;
	while(1 == fread(&node, sizeof(u64), 1, input)) {
		nodesRead++;
		//	look up node in nodes file
		unsigned long long *ptr;
	   	unsigned long long src;
		ptr = std::lower_bound(nodes, nodes+numNodes, node); 
		assert(ptr >= nodes);
		assert(ptr < nodes+numNodes);
		if(*ptr != node) {
			orphans++;
			if(orphansFile) {
				fwrite(&node, sizeof(u64), 1, orphansFile);
				src = 0;
			} else {
				abort();
			}
		} else {
			src = ptr - nodes;
		}
		
		//	write out translated edge to output
		fwrite(&src, sizeof(u64), 1, output);
		nodesWritten++;
	}
	
	fclose(input);
	fclose(output);
	if(orphansFile) {
		fclose(orphansFile);
	}
	fprintf(stderr, "read %llu nodes, wrote %llu nodes, %llu orphans\n", nodesRead, nodesWritten, orphans);
	return 0;
}
