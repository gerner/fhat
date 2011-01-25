#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#include "util.h"

int main(int argc, char **argv) {
	if(argc < 5) {
		fprintf(stderr, "retain vertex dominator size retained\n");
		return -1;
	}

	u64 numNodes;
	int vertexFd = open(argv[1], O_RDONLY);
	assert(vertexFd >= 0);
	int dominatorFd = open(argv[2], O_RDONLY);
	assert(dominatorFd >= 0);
	int sizeFd = open(argv[3], O_RDONLY);
	assert(sizeFd >= 0);
	struct stat sb;
	int retval = fstat(sizeFd, &sb);
	assert(sb.st_size % sizeof(u64) == 0);
	numNodes = sb.st_size/sizeof(u64);
	int retainedFd = openAndInitializeFile(argv[4], numNodes*sizeof(u64));
	assert(retainedFd >= 0);
	int retainedCountFd = openAndInitializeFile(argv[5], numNodes*sizeof(u64));
	assert(retainedCountFd >= 0);

	//mmap size, retained
	u64 *vertex = (u64 *)mmap(NULL, numNodes*sizeof(u64), PROT_READ, MAP_PRIVATE, vertexFd, 0);
	assert(vertex != MAP_FAILED);
	u64 *dominator = (u64 *)mmap(NULL, numNodes*sizeof(u64), PROT_READ, MAP_PRIVATE, dominatorFd, 0);
	assert(dominator != MAP_FAILED);
	u64 *size = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ, MAP_PRIVATE, sizeFd, 0);
	assert(size != MAP_FAILED);
	u64 *retained = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, retainedFd, 0);
	assert(retained != MAP_FAILED);
	u64 *retainedCount = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, retainedCountFd, 0);
	assert(retainedCount != MAP_FAILED);

	//intialize retained to the size of each instance
	bzero(retained, numNodes*sizeof(u64));
	bzero(retainedCount, numNodes*sizeof(u64));

	u64 skipped = 0;
	u64 totalSize = 0;
	u64 sizeIncludingUnreachable = 0;
	for(u64 i=numNodes-1; i>0; i--) {
		if(i % (numNodes/80) == 0) {
			fprintf(stderr, ".");
		}
		assert(i < numNodes);
		//skip nodes who claim to be the root
		//these must be unreachable nodes
		if(0 != vertex[i]) {
			u64 v = vertex[i];
			assert(v < numNodes);
			assert(dominator[v] < numNodes);
			totalSize += size[v];
			retained[v] += size[v];
			retained[dominator[v]] += retained[v];
			retainedCount[v] += 1;
			retainedCount[dominator[v]] += retainedCount[v];
		} else {
			skipped++;
		}
		sizeIncludingUnreachable += size[i];
	}
	assert(totalSize == retained[0]);
	assert(numNodes-skipped == retainedCount[0]+1);
	fprintf(stderr, "\n%llu retained in %llu reachable nodes, plus %llu unreachable in %llu unreachable nodes\n", retained[0], numNodes-skipped, sizeIncludingUnreachable - totalSize, skipped);

	retval = munmap(vertex, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(dominator, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(size, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(retained, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(retainedCount, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	
	close(vertexFd);
	close(dominatorFd);
	close(sizeFd);
	close(retainedFd);
	close(retainedCountFd);
	
	return 0;
}
