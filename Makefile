zgz: zgz.c
	gcc -Wall -O2 -lz -o zgz zgz.c

clean:
	rm -f zgz
