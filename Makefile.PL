#!/usr/bin/perl
use warnings;
use strict;
use ExtUtils::MakeMaker;

# Add a few more targets.
sub MY::postamble {
q{
all:: extra_build
clean:: extra_clean
install:: extra_install
pure_install:: extra_install

extra_build: zgz/zgz pristine-tar.spec
	pod2man -c pristine-tar pristine-tar > pristine-tar.1
	pod2man -c pristine-gz  pristine-gz  > pristine-gz.1
	pod2man -c pristine-bz2 pristine-bz2 > pristine-bz2.1
	pod2man -c zgz zgz/zgz.pod > zgz.1

ZGZ_SOURCES = zgz/zgz.c zgz/bits.c zgz/deflate.c zgz/gzip.c zgz/trees.c zgz/util.c \
	      zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES) zgz/gzip.h
	gcc -Wall -O2 -lz -o $@ $(ZGZ_SOURCES)

extra_install:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/man/man1
	install pristine-tar pristine-gz pristine-bz2 zgz/zgz $(DESTDIR)/usr/bin
	install -m 0644 *.1 $(DESTDIR)/usr/share/man/man1

extra_clean: pristine-tar.spec
	rm -f zgz/zgz *.1

pristine-tar.spec:
	sed -i "s/Version:.*/Version: $$(perl -e '$$_=<>;print m/\((.*?)\)/'<debian/changelog)/" pristine-tar.spec

.PHONY: pristine-tar.spec
}
}

WriteMakefile(
	NAME		=> 'Pristine::Tar',
	PREFIX		=> "/usr/local",
	MAN1PODS	=> {},
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
);
