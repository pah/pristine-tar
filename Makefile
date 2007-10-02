zgz: zgz.c
	gcc -O2 -DNO_COMPRESS_SUPPORT -DNO_BZIP2_SUPPORT -lz -o zgz zgz.c

clean:
	rm -f zgz
