bsd-gzip: bsd-gzip.c
	gcc -O2 -DNO_COMPRESS_SUPPORT -DNO_BZIP2_SUPPORT -lz \
		-o bsd-gzip bsd-gzip.c 
