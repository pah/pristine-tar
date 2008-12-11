build: zgz
	pod2man -c pristine-tar pristine-tar > pristine-tar.1
	pod2man -c pristine-gz  pristine-gz  > pristine-gz.1
	pod2man -c pristine-bz2 pristine-bz2 > pristine-bz2.1
	pod2man -c zgz zgz.pod > zgz.1

ZGZ_SOURCES = zgz.c bits.c deflate.c gzip.c trees.c util.c
zgz: $(ZGZ_SOURCES) gzip.h
	gcc -Wall -O2 -lz -o $@ $(ZGZ_SOURCES)

install:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/man/man1
	install pristine-tar pristine-gz pristine-bz2 zgz $(DESTDIR)/usr/bin
	install -m 0644 *.1 $(DESTDIR)/usr/share/man/man1

clean:
	rm -f zgz *.1
