#!/usr/bin/env perl
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

ZGZ_LIB:=$(PREFIX)/lib/zgz

PERL_SHEBANG:=/usr/bin/env perl
TAR_PROGRAM:=tar
XDELTA_PROGRAM:=xdelta

%:	%.in
	sed -e "s/^#!\\/usr\\/bin\\/env perl/#!$(subst /,\\/,$(PERL_SHEBANG))/g" \
	    -e "s/@TAR_PROGRAM@/$(subst /,\\/,$(TAR_PROGRAM))/g" \
	    -e "s/@XDELTA_PROGRAM@/$(subst /,\\/,$(XDELTA_PROGRAM))/g" \
	    $^ > $@


pristine-tar: pristine-tar.in Makefile.PL

pristine-gz: pristine-gz.in Makefile.PL

pristine-bz2: pristine-bz2.in Makefile.PL

pristine-xz: pristine-xz.in Makefile.PL

extra_build: zgz/zgz pristine-tar.spec pristine-tar pristine-gz pristine-bz2 pristine-xz
	pod2man -c pristine-tar pristine-tar > pristine-tar.1
	pod2man -c pristine-gz  pristine-gz  > pristine-gz.1
	pod2man -c pristine-bz2 pristine-bz2 > pristine-bz2.1
	pod2man -c pristine-xz pristine-xz > pristine-xz.1
	pod2man -c zgz zgz/zgz.pod > zgz.1
	make -C pit/suse-bzip2

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

extra_clean: pristine-tar.spec
	rm -f *.1 pristine-tar pristine-gz pristine-bz2 pristine-xz zgz/zgz
	make clean -C pit/suse-bzip2

# Extract the version number from debian/changelog to update pristine-tar.spec
pristine-tar.spec:
	sed "s/Version:.*/Version: $$(perl -e '$$_=<>;print m/\((.*?)\)/'<debian/changelog)/" pristine-tar.spec > pristine-tar.spec.tmp && mv pristine-tar.spec.tmp pristine-tar.spec

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
