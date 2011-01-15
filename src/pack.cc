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

int main(int argc, char **argv) {
	assert(argc >= 1);
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
	FILE *output = stdout;

	//for each edge
	Link edge;
	while(1 == fread(&edge, sizeof(Link), 1, input)) {
		//	look up src and tgt in nodes file
		unsigned long long src = std::lower_bound(nodes, nodes+numNodes, edge.src) - nodes;
		unsigned long long dst = std::lower_bound(nodes, nodes+numNodes, edge.dst) - nodes;
		//	write out translated edge to output
		fwrite(&src, sizeof(unsigned long long), 1, output);
		fwrite(&dst, sizeof(unsigned long long), 1, output);
	}
	
	fclose(input);
	fclose(output);
}
