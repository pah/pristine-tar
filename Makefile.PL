#!/usr/bin/env perl
use warnings;
use strict;
use ExtUtils::MakeMaker;

# Suppress warnings about parameters we allow the user to specify.
# (does not work in recent versions of MakeMaker)
$ExtUtils::MakeMaker::Recognized_Att_Keys{'PERL_SHEBANG'}   = 1;
$ExtUtils::MakeMaker::Recognized_Att_Keys{'TAR_PROGRAM'}    = 1;
$ExtUtils::MakeMaker::Recognized_Att_Keys{'XDELTA_PROGRAM'} = 1;

# read custom parameters
my %args = map { split /\s*=\s*/ } @ARGV;
my $PERL_SHEBANG   = $args{PERL_SHEBANG} || "" ;
my $TAR_PROGRAM    = $args{TAR_PROGRAM} || "" ;
my $XDELTA_PROGRAM = $args{XDELTA_PROGRAM} || "" ;

# Add a few more targets.
sub MY::postamble {

my $pa = q{
all:: extra_build
clean:: extra_clean
install:: extra_install
pure_install:: extra_install

PERL_SHEBANG  :=__PERL_SHEBANG__
TAR_PROGRAM   :=__TAR_PROGRAM__
XDELTA_PROGRAM:=__XDELTA_PROGRAM__

BIN_PERL := \
  pristine-tar \
  pristine-bz2 \
  pristine-gz  \
  pristine-xz

# Adjust scripts according to extra config parameters, if needed
ifneq (,$(PERL_SHEBANG)$(TAR_PROGRAM)$(XDELTA_PROGRAM))

ifneq (,$(PERL_SHEBANG))
PERL_SHEBANG_XPR:=-e "s|^\#!.*|\#!$(PERL_SHEBANG)|g"
endif
ifneq (,$(TAR_PROGRAM))
TAR_PROGRAM_XPR :=-e "s|tar_program = \".*\";|tar_program = \"$(TAR_PROGRAM)\";|g"
endif
ifneq (,$(XDELTA_PROGRAM))
XDELTA_PROGRAM_XPR:=-e "s|xdelta_program = \".*\";|xdelta_program = \"$(XDELTA_PROGRAM)\";|g"
endif

$(BIN_PERL): %:
	sed $(PERL_SHEBANG_XPR) \
		$(TAR_PROGRAM_XPR) \
		$(XDELTA_PROGRAM_XPR) \
		$@ > $@.new
	mv $@.new $@
	chmod a+x $@
PHONY+=$(BIN_PERL)

endif

extra_build: zgz/zgz zgz.1 pristine-tar.spec
	$(MAKE) -C pit/suse-bzip2

ZGZ_LIB:=$(PREFIX)/lib/zgz
ZGZ_SOURCES = zgz/zgz.c zgz/gzip/*.c zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES)
	gcc -Wall -O2 -o $@ $(ZGZ_SOURCES) -lz -DZGZ_LIB=\"$(ZGZ_LIB)\"

zgz.1: zgz/zgz.pod
	pod2man -c zgz $< > $@

extra_install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install zgz/zgz $(DESTDIR)$(PREFIX)/bin
	install -m 0644 zgz.1 $(DESTDIR)$(PREFIX)/share/man/man1
	install -d $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/bzip2 $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/libbz2* $(DESTDIR)$(ZGZ_LIB)/suse-bzip2

extra_clean: pristine-tar.spec
	$(MAKE) clean -C pit/suse-bzip2

# Extract the version number from debian/changelog to update pristine-tar.spec
pristine-tar.spec:
	sed "s/Version:.*/Version: $$(perl -e '$$_=<>;print m/\((.*?)\)/'<debian/changelog)/" \
	    pristine-tar.spec > pristine-tar.spec.new \
        && mv pristine-tar.spec.new pristine-tar.spec
PHONY+=pristine-tar.spec

.PHONY: $(PHONY)
};

	# replace custom parameters
	$pa =~ s|__PERL_SHEBANG__|${PERL_SHEBANG}|eg;
	$pa =~ s|__TAR_PROGRAM__|${TAR_PROGRAM}|eg;
	$pa =~ s|__XDELTA_PROGRAM__|${XDELTA_PROGRAM}|eg;

	$pa;
}

WriteMakefile(
	NAME		=> 'Pristine',
	AUTHOR		=> 'Joey Hess <joey@kitenet.net>',
	ABSTRACT	=> 'regenerate a pristine upstream tarball using only a small '
					.'binary delta file and a copy of the source which can be '
					.'a revision control checkout',
	PREFIX		=> "/usr/local",
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
	EXE_FILES	=> ["pristine-tar","pristine-bz2","pristine-gz","pristine-xz"],
	clean		=> { FILES => 'zgz/zgz zgz.1' },
);
