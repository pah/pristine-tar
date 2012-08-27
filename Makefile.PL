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

ZGZ_LIB=$(PREFIX)/lib/zgz

extra_build: zgz/zgz pristine-tar.spec
	$(MAKE) -C pit/suse-bzip2 PREFIX=$(PREFIX)

ZGZ_SOURCES = zgz/zgz.c zgz/gzip/*.c zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES)
	gcc -Wall -O2 -o $@ $(ZGZ_SOURCES) -lz -DZGZ_LIB=\"$(ZGZ_LIB)\"

extra_install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install zgz/zgz $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/bzip2 $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/libbz2* $(DESTDIR)$(ZGZ_LIB)/suse-bzip2

extra_clean:
	$(MAKE) clean -C pit/suse-bzip2 PREFIX=$(PREFIX)

pristine-tar.spec: debian/changelog
	sed "s/Version:.*/Version: $$($(PERLRUN) -e '$$_=<>;print m/\((.*?)\)/'<$<)/" \
		$@ > $@.new && $(MV) $@.new $@

.PHONY: pristine-tar.spec
}
}

WriteMakefile(
	NAME		=> 'Pristine',
	AUTHOR		=> 'Joey Hess <joey@kitenet.net>',
	ABSTRACT	=>
		'regenerate a pristine upstream tarball using only a small '.
		'binary delta file and a revision control checkout',
	MAN1PODS	=> {
		'pristine-tar' => '$(INST_MAN1DIR)/pristine-tar.1',
		'pristine-bz2' => '$(INST_MAN1DIR)/pristine-bz2.1',
		'pristine-gz'  => '$(INST_MAN1DIR)/pristine-gz.1',
		'pristine-xz'  => '$(INST_MAN1DIR)/pristine-xz.1',
		'zgz/zgz.pod'  => '$(INST_MAN1DIR)/zgz.1'
	},
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
	EXE_FILES	=> ["pristine-tar","pristine-bz2","pristine-gz","pristine-xz"],
	clean		=> { FILES => 'zgz/zgz' },
);
