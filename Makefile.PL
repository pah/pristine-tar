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

ZOMBIES=zgz/pit/suse-bzip2/suse-bzip2
BINS=zgz/zgz $(ZOMBIES)
ZGZ_LIB=$(PREFIX)/lib/zgz

extra_build: $(BINS) pristine-tar.spec
	pod2man -c pristine-tar pristine-tar > pristine-tar.1
	pod2man -c pristine-gz  pristine-gz  > pristine-gz.1
	pod2man -c pristine-bz2 pristine-bz2 > pristine-bz2.1
	pod2man -c zgz zgz/zgz.pod > zgz.1

ZGZ_SOURCES = zgz/zgz.c zgz/gzip/*.c zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES)
	gcc -Wall -O2 -o $@ $(ZGZ_SOURCES) -lz -DZGZ_LIB=\"$(ZGZ_LIB)\"

SUSE_BZIP2_SOURCES = zgz/pit/suse-bzip2/*.c
zgz/pit/suse-bzip2/suse-bzip2: $(SUSE_BZIP2_SOURCES)
	gcc -Wall -O2 -o $@ $(SUSE_BZIP2_SOURCES)

extra_install:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/man/man1
	install pristine-tar pristine-gz pristine-bz2 zgz/zgz $(DESTDIR)/usr/bin
	install -m 0644 *.1 $(DESTDIR)/usr/share/man/man1
	install -d $(DESTDIR)$(ZGZ_LIB)
	install $(ZOMBIES) $(DESTDIR)$(ZGZ_LIB)

extra_clean: pristine-tar.spec
	rm -f *.1 $(BINS)

pristine-tar.spec:
	sed -i "s/Version:.*/Version: $$(perl -e '$$_=<>;print m/\((.*?)\)/'<debian/changelog)/" pristine-tar.spec

.PHONY: pristine-tar.spec
}
}

WriteMakefile(
	NAME		=> 'Pristine',
	PREFIX		=> "/usr/local",
	MAN1PODS	=> {},
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
);
