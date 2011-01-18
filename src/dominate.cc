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
#include <map>
#include <vector>
#include "util.h"

Link *pred;
unsigned long long *semi;
unsigned long long *vertex;
unsigned long long *parent;

//this will hold the immediate dominator for each node
unsigned long long *dominator;

//this will temporarily hold ancestors and labels for LINK/EVAL/COMPRESS
unsigned long long *ancestor;
unsigned long long *label;

//TODO: is this the best choice of data structure?
std::map<unsigned long long, std::vector<unsigned long long> > bucket;

void COMPRESS(unsigned long long v) {
	assert(0 != ancestor[v]);
	if(0 != ancestor[ancestor[v]]) {
		COMPRESS(ancestor[v]);
		if(semi[label[ancestor[v]]] < semi[label[v]]) {
			label[v] = label[ancestor[v]];
		}
		ancestor[v] = ancestor[ancestor[v]];
	}
}

unsigned long long EVAL(unsigned long long v) {
	if(0 == ancestor[v]) {
		return v;
	} else {
		COMPRESS(v);
		return label[v];
	}
}

void LINK(unsigned long long v, unsigned long long w) {
	ancestor[w] = v;
}

int main(int argc, char **argv) {
	if(argc < 6) {
		fprintf(stderr, "dominate pred semi vertex parent dominator\n");
		return -1;
	}

	int predFd, semiFd, vertexFd, parentFd, dominatorFd;
	predFd = open(argv[1], O_RDONLY);
	semiFd = open(argv[2], O_RDWR);
	vertexFd = open(argv[3], O_RDONLY);
	parentFd = open(argv[4], O_RDONLY);
	
	int retval;
	struct stat sb;
	retval = fstat(predFd, &sb);
	assert(-1 != retval);
	assert(sb.st_size % sizeof(Link) == 0);
	unsigned long long numEdges = sb.st_size / sizeof(Link);
	retval = fstat(semiFd, &sb);
	assert(-1 != retval);
	assert(sb.st_size % sizeof(unsigned long long) == 0);
	unsigned long long numNodes = sb.st_size / sizeof(unsigned long long);

	dominatorFd = openAndInitializeFile(argv[5], numNodes * sizeof(unsigned long long));

	char ancestorName[32];
	strcpy(ancestorName, "/tmp/ancestorXXXXXX");
	char labelName[32];
	strcpy(labelName, "/tmp/labelXXXXXX");
	int ancestorFd, labelFd;
	ancestorFd = initializeFile(mkstemp(ancestorName), numNodes * sizeof(unsigned long long));
	labelFd = initializeFile(mkstemp(labelName), numNodes * sizeof(unsigned long long));

	//set up semi, vertex, parent, pred, dominator via mmap
	pred = (Link *)mmap(NULL, numEdges*sizeof(Link), PROT_READ, MAP_PRIVATE, predFd, 0);
	assert(pred != MAP_FAILED);
	semi = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, semiFd, 0);
	assert(semi != MAP_FAILED);
	vertex = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ, MAP_PRIVATE, vertexFd, 0);
	assert(vertex != MAP_FAILED);
	parent = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ, MAP_PRIVATE, parentFd, 0);
	assert(parent != MAP_FAILED);
	dominator = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, dominatorFd, 0);
	assert(dominator != MAP_FAILED);
	
	ancestor = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, ancestorFd, 0);
	assert(ancestor != MAP_FAILED);
	label = (unsigned long long *)mmap(NULL, numNodes*sizeof(unsigned long long), PROT_READ | PROT_WRITE, MAP_SHARED, labelFd, 0);
	assert(label != MAP_FAILED);

	fprintf(stderr, "initializing dominator\n");
	//initialize dominator to all zeros
	bzero(dominator, numNodes*sizeof(unsigned long long));

	bzero(ancestor, numNodes*sizeof(unsigned long long));
	for(unsigned long long j=0; j<numNodes; j++) {
		label[j] = j;
	}

	fprintf(stderr, "steps 2 and 3: computing semidominators and implicitly defining immediate dominators\n");
	unsigned long long skipped = 0;
	unsigned long long computed = 0;
	unsigned long long edgesConsidered = 0;
	unsigned long long i;
	unsigned long long dominatorSets = 0;
	unsigned long long semidominatorSets = 0;
	for(i = numNodes-1; i>0; i--) {
		if(i % (numNodes/80) == 0) {
			fprintf(stderr, ".");
		}
		unsigned long long w = vertex[i];
		if(w) {
			assert(i == semi[w]);
			Link *ptr = std::lower_bound(pred, pred+numEdges, Link(w, 0));
			assert(ptr < pred+numEdges);
			assert(ptr == pred || *(ptr-1) < *ptr);
//			if(ptr->src != w) {
//				fprintf(stderr, "%llu didn't find any pred %llu (%llu->%llu) semi[w] == %llu\n", i, w, (ptr-1)->dst, (ptr-1)->src, semi[w]);
				assert(ptr->src == w);
//			}
			std::map<unsigned long long, std::vector<unsigned long long> >::iterator bucketPos;
			while(ptr < pred+numEdges && ptr->src == w) {
				assert(ptr == pred || *(ptr-1) < *ptr);
				assert(ptr->dst < numNodes);
				edgesConsidered++;
				unsigned long long u = EVAL(ptr->dst);
				if(semi[u] < semi[w]) {
					semi[w] = semi[u];
					semidominatorSets++;
				}
				ptr++;
			}
		
			bucketPos = bucket.find(vertex[semi[w]]);
			if(bucketPos == bucket.end()) {
				bucket[vertex[semi[w]]] = std::vector<unsigned long long>();
			}
			bucket[vertex[semi[w]]].push_back(w);
			LINK(parent[w], w);

			bucketPos = bucket.find(parent[w]);
			if(bucketPos != bucket.end()) {
				assert(bucketPos->first == parent[w]);
				while(!bucketPos->second.empty()) {
					unsigned long long v = bucketPos->second.back();
					bucketPos->second.pop_back();
					unsigned long long u = EVAL(v);
					if(semi[u] < semi[v]) {
						dominator[v] = u;
						dominatorSets++;
					} else {
						dominator[v] = parent[w];
						dominatorSets++;
					}
				}
				bucket.erase(bucketPos);
			}
			computed++;
		} else {
			skipped++;
		}
	}
	fprintf(stderr, "\n");
	
	fprintf(stderr, "step 4: explicitly defining immediate dominators\n");
	unsigned long long nonZeroDominators = 0;
	for(i=1; i<numNodes; i++) {
		unsigned long long w = vertex[i];
		if(dominator[w] != vertex[semi[w]]) {
			dominator[w] = dominator[dominator[w]];
			dominatorSets++;
		}
		if(0 != dominator[w]) {
			nonZeroDominators++;
		}
	}
	//of course. (technically we don't have to do this since we initialized it earlier)
	dominator[0] = 0;
	fprintf(stderr, "%llu non-zero dominators, computed %llu, skipped %llu unreachable nodes, %llu edges considered\n", nonZeroDominators, computed, skipped, edgesConsidered);


	retval = munmap(pred, numEdges*sizeof(Link));
	assert(retval != -1);
	retval = munmap(semi, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(vertex, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(parent, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(dominator, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(ancestor, numNodes*sizeof(unsigned long long));
	assert(retval != -1);
	retval = munmap(label, numNodes*sizeof(unsigned long long));
	assert(retval != -1);

	close(predFd);
	close(semiFd);
	close(vertexFd);
	close(parentFd);
	close(dominatorFd);
	close(ancestorFd);
	close(labelFd);

	remove(ancestorName);
	remove(labelName);

	return 0;
}
