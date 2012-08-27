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

ZGZ_LIB=$(PREFIX)/lib/zgz

extra_build: zgz/zgz pristine-tar.spec
	pod2man -c pristine-tar pristine-tar > pristine-tar.1
	pod2man -c pristine-gz  pristine-gz  > pristine-gz.1
	pod2man -c pristine-bz2 pristine-bz2 > pristine-bz2.1
	pod2man -c pristine-xz pristine-xz > pristine-xz.1
	pod2man -c zgz zgz/zgz.pod > zgz.1
	$(MAKE) -C pit/suse-bzip2 PREFIX=$(PREFIX)

ZGZ_SOURCES = zgz/zgz.c zgz/gzip/*.c zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES)
	gcc -Wall -O2 -o $@ $(ZGZ_SOURCES) -lz -DZGZ_LIB=\"$(ZGZ_LIB)\"

SUSE_BZIP2_SOURCES = pit/suse-bzip2/*.c
pit/suse-bzip2/suse-bzip2: $(SUSE_BZIP2_SOURCES)
	gcc -Wall -O2 -o $@ $(SUSE_BZIP2_SOURCES)

extra_install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install pristine-tar pristine-gz pristine-bz2 pristine-xz zgz/zgz $(DESTDIR)$(PREFIX)/bin
	install -m 0644 *.1 $(DESTDIR)$(PREFIX)/share/man/man1
	install -d $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/bzip2 $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/libbz2* $(DESTDIR)$(ZGZ_LIB)/suse-bzip2

extra_clean:
	rm -f *.1 zgz/zgz
	$(MAKE) clean -C pit/suse-bzip2 PREFIX=$(PREFIX)

pristine-tar.spec:
	sed -i "s/Version:.*/Version: $$(perl -e '$$_=<>;print m/\((.*?)\)/'<debian/changelog)/" pristine-tar.spec

.PHONY: pristine-tar.spec
}
}

WriteMakefile(
	NAME		=> 'Pristine',
	MAN1PODS	=> {},
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
);
