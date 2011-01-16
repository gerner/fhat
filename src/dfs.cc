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
	Link(unsigned long long src, unsigned long long dst) {
		this->src = src;
		this->dst = dst;
	}
};

bool operator<(const Link &lhs, const Link &rhs) {
	if (lhs.src < rhs.src) {
		return true;
	} else if (lhs.src == rhs.src) {
		return lhs.dst < rhs.dst;
	}
	return false;
}

unsigned long long *semi;
unsigned long long *vertex;
unsigned long long *parent;
unsigned long long numNodes;

Link *edges;
unsigned long long numEdges;

unsigned long long edgesConsidered = 0;

unsigned long long dfs(unsigned long long v, unsigned long long n) {
	if(n % (numNodes / 25) == 0) {
		fprintf(stderr, "discovered node %llu: %llu\n", n, v);
	}
	unsigned long long descendants = 1;
	semi[v] = n;
	vertex[n] = v;
	
	//this does a binary search to find the edges, which is probably fine, but this is somewhat random I/O
	Link *ptr = std::lower_bound(edges, edges+numEdges, Link(v, 0));
	//for each edge (v,w) out of v
	while(ptr < edges+numEdges && ptr->src == v) {
		assert((ptr == edges || *(ptr-1) < *ptr));
//		fprintf(stderr, "considering (%llu, %llu)\n", ptr->src, ptr->dst);
		assert(ptr->dst < numNodes);
		if(semi[ptr->dst] == 0) {
			parent[ptr->dst] = v;
			descendants += dfs(ptr->dst, n+1);
		} else {
//			fprintf(stderr, "%llu already discovered\n", ptr->dst);
		}
		edgesConsidered++;
		ptr++;
	}
	return descendants;
}

int openAndInitializeFile(const char *name, size_t size) {
	int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	assert(fd);
	int retval = lseek(fd, size-1, SEEK_SET);
	assert(size == retval+1);
	retval = write(fd, "", 1);
	assert(1 == retval);
	retval = lseek(fd, 0, SEEK_SET);
	assert(0 == retval);
	return fd;
}

int main(int argc, char **argv) {
	if(argc < 5) {
		fprintf(stderr, "dfs edges semi vertex parent numnodes\n");
		return 0;
	}
	assert(argc > 4);

	numNodes = (unsigned long long)atoi(argv[5]);

	int edgesFd, semiFd, vertexFd, parentFd;
	edgesFd = open(argv[1], O_RDONLY);
	semiFd = openAndInitializeFile(argv[2], numNodes * sizeof(unsigned long long));
	vertexFd = openAndInitializeFile(argv[3], numNodes * sizeof(unsigned long long));
	parentFd = openAndInitializeFile(argv[4], numNodes * sizeof(unsigned long long));

	int retval;
	struct stat sb;
	retval = fstat(edgesFd, &sb);
	assert(sb.st_size % sizeof(Link) == 0);
	numEdges = sb.st_size / sizeof(Link);
	edges = (Link*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, edgesFd, 0);
	
	//set up semi, vertex, parent, edges via mmap
	semi = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, semiFd, 0);
	assert(semi != MAP_FAILED);
	vertex = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, vertexFd, 0);
	assert(semi != MAP_FAILED);
	parent = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, parentFd, 0);
	assert(semi != MAP_FAILED);
	
	fprintf(stderr, "initializing semi\n");
	//initialize semi to all zeros
	bzero(semi, numNodes*sizeof(unsigned long long));
	bzero(vertex, numNodes*sizeof(unsigned long long));
	bzero(parent, numNodes*sizeof(unsigned long long));

	fprintf(stderr, "building spanning tree by DFS\n");
	unsigned long long descendants = dfs(0, 0);
	assert(descendants < numNodes);
	fprintf(stderr, "%llu reachable nodes, %llu unreachable, %llu edges considered, %llu edges total\n", descendants, numNodes-descendants, edgesConsidered, numEdges);
	
	retval = munmap(semi, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(vertex, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(parent, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(edges, sb.st_size);
	assert(retval != -1);

	close(semiFd);
	close(vertexFd);
	close(parentFd);
	close(edgesFd);

	//parent is now a spanning tree for the graph!
	//the entries of semi that are zero are unreachable from the root
	
	return 0;
}
