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

struct Link {
	unsigned long long src;
	unsigned long long dst;
};

bool operator<(const Link &lhs, const Link &rhs) {
	if (lhs.src < rhs.src) {
		return true;
	} else if (lhs.src == rhs.src) {
		return lhs.dst < rhs.dst;
	}
	return false;
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
	assert(sb.st_size % sizeof(unsigned long long) == 0);
	size_t numNodes = sb.st_size / sizeof(unsigned long long);
	assert(0 == retval);
	//mmap nodes file
	unsigned long long *nodes = (unsigned long long*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, nodesFd, 0);
	
	FILE *input = stdin;
	if(argc > 2) {
		input = fopen(argv[2], "r");
	}
	assert(input);
	FILE *output = stdout;
	if(argc > 3) {
		output = fopen(argv[3], "r");
	}
	FILE *orphansFile = fopen("packorphans", "w");

	unsigned long long orphans=0;
	unsigned long long edgesRead=0;
	unsigned long long edgesWritten=0;

	//for each edge
	Link last;
	last.src = 0;
	last.dst = 0;
	Link lastOut = last;
	Link edge;
	while(1 == fread(&edge, sizeof(Link), 1, input)) {
		assert((last.src == 0 && last.dst == 0) || last < edge);
		edgesRead++;
		//	look up src and tgt in nodes file
		unsigned long long *ptr;
	   	unsigned long long src;
		ptr = std::lower_bound(nodes, nodes+numNodes, edge.src); 
		assert(ptr >= nodes);
		assert(ptr < nodes+numNodes);
		if(!(0 == edge.src || *ptr == edge.src)) {
			fprintf(stderr, "orphan (%llu,%llu) (src)\n", edge.src, edge.dst);
			orphans++;
			if(orphansFile) {
				fwrite(&edge, sizeof(Link), 1, orphansFile);
			}
			//it's really bad to lose the source of a reference
			abort();	
		} else {
			src = ptr - nodes;
		}
		
		unsigned long long dst;
		ptr = std::lower_bound(nodes, nodes+numNodes, edge.dst);
		assert(ptr >= nodes);
		assert(ptr < nodes+numNodes);
		if(!(0 == edge.dst || *ptr == edge.dst)) {
			//fprintf(stderr, "orphan (%llu,%llu) (dst) (%llu: %llu)\n", edge.src, edge.dst, *ptr, (unsigned long long)(ptr-nodes));
			orphans++;
			if(orphansFile) {
				fwrite(&edge, sizeof(Link), 1, orphansFile);
			}
			//not sure what this means, but it's basically like a null reference I guess
			continue;
		} else {
			dst = ptr - nodes;
		}

		Link nextOut;
		nextOut.src = src;
		nextOut.dst = dst;
		assert((lastOut.src == 0 && lastOut.dst == 0) || lastOut < nextOut);
		//	write out translated edge to output
		fwrite(&src, sizeof(unsigned long long), 1, output);
		fwrite(&dst, sizeof(unsigned long long), 1, output);
		edgesWritten++;
		last = edge;
		lastOut = nextOut;
	}
	
	fclose(input);
	fclose(output);
	if(orphansFile) {
		fclose(orphansFile);
	}
	fprintf(stderr, "read %llu edges, wrote %llu edges, %llu orphans\n", edgesRead, edgesWritten, orphans);
	return 0;
}
