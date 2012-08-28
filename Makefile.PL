#!/usr/bin/env perl
use warnings;
use strict;
use ExtUtils::MakeMaker;

# read custom parameters
my %args = map { split /\s*=\s*/ } @ARGV;
my $PERL_SHEBANG   = $args{PERL_SHEBANG} || "" ;
my $TAR_PROGRAM    = $args{TAR_PROGRAM} || "" ;
my $XDELTA_PROGRAM = $args{XDELTA_PROGRAM} || "" ;

if ($PERL_SHEBANG || $TAR_PROGRAM || $XDELTA_PROGRAM)
{
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

    $subst .= '-e "s|^\#!.*|\#!$(PERL_SHEBANG)|g"'
        unless ! ${PERL_SHEBANG};
    $subst .= q[ -e "s|tar_program = \".*\";|tar_program = \"$(TAR_PROGRAM)\";|g"]
        unless ! ${TAR_PROGRAM};
    $subst .= q[ -e "s|xdelta_program = \".*\";|xdelta_program = \"$(XDELTA_PROGRAM)\";|g"]
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

ZGZ_LIB:=$(PREFIX)/lib/zgz
ZGZ_SOURCES = zgz/zgz.c zgz/gzip/*.c zgz/old-bzip2/*.c
zgz/zgz: $(ZGZ_SOURCES)
	gcc -Wall -O2 -o $@ $(ZGZ_SOURCES) -lz -DZGZ_LIB=\"$(ZGZ_LIB)\"

extra_build: zgz/zgz pristine-tar.spec
	$(MAKE) PREFIX=$(PREFIX) -C pit/suse-bzip2

extra_install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install zgz/zgz $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/bzip2   $(DESTDIR)$(ZGZ_LIB)/suse-bzip2
	install pit/suse-bzip2/libbz2* $(DESTDIR)$(ZGZ_LIB)/suse-bzip2

extra_clean: pristine-tar.spec
	$(MAKE) PREFIX=$(PREFIX) clean -C pit/suse-bzip2

# Extract the version number from debian/changelog to update pristine-tar.spec
pristine-tar.spec:
	sed "s/Version:.*/Version: $$(perl -e '$$_=<>;print m/\((.*?)\)/'<debian/changelog)/" \
	    pristine-tar.spec > pristine-tar.spec.new \
        && mv pristine-tar.spec.new pristine-tar.spec
PHONY+=pristine-tar.spec

.PHONY: $(PHONY)
}

}

ExtUtils::MakeMaker::WriteMakefile(
	NAME		=> 'Pristine',
	AUTHOR		=> 'Joey Hess <joey@kitenet.net>',
	ABSTRACT	=> 'regenerate a pristine upstream tarball using only a small '
				  .'binary delta file and a revision control checkout',
	MAN1PODS  => { 'pristine-tar' => '$(INST_MAN1DIR)/pristine-tar.1',
				   'pristine-bz2' => '$(INST_MAN1DIR)/pristine-bz2.1',
				   'pristine-gz'  => '$(INST_MAN1DIR)/pristine-gz.1',
				   'pristine-xz'  => '$(INST_MAN1DIR)/pristine-xz.1',
				   'zgz/zgz.pod'  => '$(INST_MAN1DIR)/zgz.1' },
	MAN3PODS	=> {},
	PMLIBDIRS	=> ["Pristine"],
	EXE_FILES	=> ["pristine-tar","pristine-bz2","pristine-gz","pristine-xz"],
	macro		=> { PERL_SHEBANG   => "${PERL_SHEBANG}",
					 TAR_PROGRAM    => "${TAR_PROGRAM}",
					 XDELTA_PROGRAM => "${XDELTA_PROGRAM}" },
	clean		=> { FILES => 'zgz/zgz' },
);
