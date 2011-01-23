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
};

bool operator<(const Link &lhs, const Link &rhs) {
	if (lhs.src < rhs.src) {
		return true;
	} else if (lhs.src == rhs.src) {
		return lhs.dst < rhs.dst;
	}
	return false;
}

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
