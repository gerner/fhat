#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

typedef unsigned long long u64;

struct Link {
	unsigned long long src;
	unsigned long long dst;
	Link() {
	}
	Link(unsigned long long src, unsigned long long dst) {
		this->src = src;
		this->dst = dst;
	}
	bool operator<(const Link &rhs) const {
		if (this->src < rhs.src) {
			return true;
		} else if (this->src == rhs.src) {
			return this->dst < rhs.dst;
		}
		return false;
	}
	bool operator>(const Link &rhs) const {
		if (this->src > rhs.src) {
			return true;
		} else if (this->src == rhs.src) {
			return this->dst > rhs.dst;
		}
		return false;
	}
};

template <int N>
struct U64_N {
	u64 v[N];

	bool operator<(const U64_N<N> &rhs) const {
		for(size_t i=0; i<N; i++) {
			if(this->v[i] < rhs.v[i]) {
				return true;
			} else if(this->v[i] != rhs.v[i]) {
				return false;
			}
		}
		return false;
	}
	
	bool operator>(const U64_N<N> &rhs) const {
		for(size_t i=0; i<N; i++) {
			if(this->v[i] > rhs.v[i]) {
				return true;
			} else if(this->v[i] != rhs.v[i]) {
				return false;
			}
		}
		return false;
	}
};

int initializeFile(int fd, size_t size) {
	assert(fd);
	int retval = lseek(fd, size-1, SEEK_SET);
	assert(size == (size_t)(retval+1));
	retval = write(fd, "", 1);
	assert(1 == retval);
	retval = lseek(fd, 0, SEEK_SET);
	assert(0 == retval);
	return fd;
}

int openAndInitializeFile(const char *name, size_t size) {
	int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	return initializeFile(fd, size);
}

template <class T>
int readRecord(T *buf, FILE *in) {
	return fread(buf, sizeof(T), 1, in);
}

template <class T>
int writeRecord(T *buf, FILE *out) {
	return fwrite(buf, sizeof(T), 1, out);
}

#endif
