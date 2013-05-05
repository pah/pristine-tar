#!/usr/bin/env perl
use warnings;
use strict;
use ExtUtils::MakeMaker;

# read custom parameters
my %args = map { split /\s*=\s*/ } @ARGV;
my $PERL_SHEBANG   = $args{PERL_SHEBANG} || "" ;
my $TAR_PROGRAM    = $args{TAR_PROGRAM} || "" ;
my $XDELTA_PROGRAM = $args{XDELTA_PROGRAM} || "" ;

if ($PERL_SHEBANG || $TAR_PROGRAM || $XDELTA_PROGRAM) {
  print <<EOF;
Info: MakeMaker may warn about the custom parameters 'PERL_SHEBANG',
      'TAR_PROGRAM', and 'XDELTA_PROGRAM', which are anyway correctly
      handled for generating the Makefile.
EOF
}

package MY;

sub macro {
    my $inherited = shift->SUPER::macro(@_);
    my $subst = '';

    $subst .= q[-e "s|^\#!.*|\#!$(PERL_SHEBANG)|g" ]
        unless ! ${PERL_SHEBANG};
    $subst .= q[-e "s|tar_program = \".*\";|tar_program = \"$(TAR_PROGRAM)\";|g" ]
        unless ! ${TAR_PROGRAM};
    $subst .= q[-e "s|xdelta_program = \".*\";|xdelta_program = \"$(XDELTA_PROGRAM)\";|g" ]
        unless ! ${XDELTA_PROGRAM};

    $inherited .= "\nPARAM_SUBST = ${subst}";
    $inherited;
}
# Add a few more targets.
sub postamble {
q{
all:: extra_build
clean:: extra_clean
install:: extra_install
pure_install:: extra_install

ifneq (,$(PARAM_SUBST))
$(EXE_FILES): %:
	sed $(PARAM_SUBST) $@ > $@.new
	$(MV) $@.new $@
	$(CHMOD) $(PERM_RWX) $@
PHONY+=$(EXE_FILES)
endif

LIBDIR ?= $(PREFIX)/lib
PKGLIBDIR ?= $(LIBDIR)/pristine-tar

extra_build: zgz/zgz pristine-tar.spec
	pod2man -c pristine-tar pristine-tar > pristine-tar.1
	pod2man -c pristine-gz  pristine-gz  > pristine-gz.1
	pod2man -c pristine-bz2 pristine-bz2 > pristine-bz2.1
	pod2man -c pristine-xz pristine-xz > pristine-xz.1
	pod2man -c zgz zgz/zgz.pod > zgz.1
	$(MAKE) -C pit/suse-bzip2 PREFIX=$(PREFIX)

ZGZ_SOURCES = zgz/zgz.c zgz/gzip/*.c zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES)
	gcc -Wall -O2 -o $@ $(ZGZ_SOURCES) -lz -DPKGLIBDIR=\"$(PKGLIBDIR)\"

extra_install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install zgz/zgz $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 *.1 $(DESTDIR)$(PREFIX)/share/man/man1
	install -d $(DESTDIR)$(PKGLIBDIR)/suse-bzip2
	install pit/suse-bzip2/bzip2 pit/suse-bzip2/libbz2* $(DESTDIR)$(PKGLIBDIR)/suse-bzip2

extra_clean:
	$(MAKE) clean -C pit/suse-bzip2 PREFIX=$(PREFIX)

pristine-tar.spec: debian/changelog
	sed "s/Version:.*/Version: $$($(PERLRUN) -e '$$_=<>;print m/\((.*?)\)/'<$<)/" \
		$@ > $@.new && $(MV) $@.new $@
PHONY+=pristine-tar.spec

.PHONY: $(PHONY)
}
}

ExtUtils::MakeMaker::WriteMakefile(
	NAME		=> 'Pristine',
	AUTHOR		=> 'Joey Hess <joey@kitenet.net>',
	ABSTRACT	=>
		'regenerate a pristine upstream tarball using only a small '.
		'binary delta file and a revision control checkout',
	MAN1PODS	=> {},
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
	EXE_FILES	=> ["pristine-tar","pristine-bz2","pristine-gz","pristine-xz"],
	macro		=> {
		PERL_SHEBANG   => "${PERL_SHEBANG}",
		TAR_PROGRAM    => "${TAR_PROGRAM}",
		XDELTA_PROGRAM => "${XDELTA_PROGRAM}"
	},
	clean		=> { FILES => 'zgz/zgz' },
);
