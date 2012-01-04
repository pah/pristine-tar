#include <stdio.h>
#include <stdlib.h>

extern void suse_bzip2(int level);

int main(int argc, char **argv) {
	int level;
	if (argc != 2) {
		fprintf(stderr, "specify compression level parameter\n");
		exit(1);
	}
	level = atoi(argv[1]);
	if (level < 1 || level > 9) {
		fprintf(stderr, "1-9 only\n");
		exit(1);
	}
	suse_bzip2(level);
	exit(0);
}
