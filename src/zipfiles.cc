#include <string.h>
#include <vector>

#include "util.h"

int zip(std::vector<FILE *> in, FILE *out) {
	assert(in.size() > 0);
	u64 v;
	while(1 == readRecord(&v, in[0])) {
		writeRecord(&v, out);
		for(size_t i=1; i<in.size(); i++) {
			int retval = readRecord(&v, in[i]);
			assert(1 == retval);
			writeRecord(&v, out);
		}
	}
	return 0;
}

int unzip(FILE *in, std::vector<FILE *> out) {
	assert(out.size() > 0);
	u64 v;
	while(1 == readRecord(&v, in)) {
		writeRecord(&v, out[0]);
		for(size_t i=1; i<out.size(); i++) {
			int retval = readRecord(&v, in);
			assert(1 == retval);
			writeRecord(&v, out[i]);
		}
	}
	return 0;
}

int main(int argc, char **argv) {
	//zip:   open n files of u64 and output one
	//unzip: open one file of u64s and output n
	
	assert(argc > 2);
	
	if(0 == strcmp(argv[1], "zip")) {
		std::vector<FILE *> in;
		FILE *out = stdout;
		int i;
		for(i=2; i<argc; i++){
			FILE *f = fopen(argv[i], "r");
			assert(f);
			in.push_back(f);
		}
		int retval = zip(in, out);
		fclose(out);
		return retval;
	} else if(0 == strcmp(argv[1], "unzip")) {
		FILE *in = stdin;
		std::vector<FILE *> out;
		int i;
		for(i=2; i<argc; i++){
			FILE *f = fopen(argv[i], "w");
			assert(f);
			out.push_back(f);
		}
		int retval = unzip(in, out);
		for(size_t j=0; j<out.size(); j++) {
			fclose(out[j]);
		}
		return retval;
	}

	return -1;
}
