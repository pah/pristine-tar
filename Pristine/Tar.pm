#!/usr/bin/perl
# pristine-tar utility library

package Pristine::Tar;

use warnings;
use strict;
use File::Temp;
use Getopt::Long;
use Exporter q{import};

our @EXPORT = qw(error message debug vprint doit doit_redir tempdir dispatch
	$verbose $debug $keep);

our $verbose=0;
our $debug=0;
our $keep=0;

sub error {
	die "$0: @_\n";
}

sub message {
	print STDERR "$0: @_\n";
}

sub debug {
	message(@_) if $debug;
}

sub vprint {
	message(@_) if $verbose;
}

sub doit {
	vprint(@_);
	if (system(@_) != 0) {
		error "command failed: @_";
	}
}

sub doit_redir {
	no warnings 'once';
	my ($in, $out, @args) = @_;
	vprint(@args, "<", $in, ">", $out);
	open INFILE, "<", $in or die("Could not open '$in' for reading: $!\n");
	open OUTFILE, ">", $out or die("Could not open '$out' for reading: $!\n");
	my $pid = open2(">&OUTFILE", "<&INFILE", @args);
	waitpid $pid, 0;
}

sub tempdir {
	return File::Temp::tempdir("pristine-tar.XXXXXXXXXX",
		TMPDIR => 1, CLEANUP => !$keep);
}

# Workaround for bug #479317 in perl 5.10.
sub END {
	chdir("/");
}

sub dispatch {
	my %params=@_;

	my %commands=%{$params{commands}};
	my %options=%{$params{options}} if exists $params{options};

	my $run=sub {
		my $command=shift;
		my $i=$commands{$command};
		if (! defined $i) {
			error "Unknown subcommand \"$command\"";
		}

		# Check that the right number of args were passed by user.
		if (defined $i->[1] && @_ != $i->[1]) {
			$command="usage";
			$i=$commands{$command};
		}

		$i->[0]->(@_);

		exit 1 if $command eq "usage";
	};

	Getopt::Long::Configure("bundling");
	if (! GetOptions(%options,
			"v|verbose!" => \$verbose,
			"d|debug!" => \$debug,
			"k|keep!" => \$keep) ||
	    ! @ARGV) {
	    	$run->("usage");
	}

	$run->(@ARGV);
}


# magic identification
use constant GZIP_ID1            => 0x1F;
use constant GZIP_ID2            => 0x8B;
use constant BZIP2_ID1           => 0x42;
use constant BZIP2_ID2           => 0x5a;

# compression methods
# 0x00-0x07 are reserved
use constant GZIP_METHOD_DEFLATE => 0x08;
# 'h' for Bzip2 ('H'uffman coding)
use constant BZIP2_METHOD_HUFFMAN => 0x68;

# flags
use constant {
	GZIP_FLAG_FTEXT		 => 0,
	GZIP_FLAG_FHCRC		 => 1,
	GZIP_FLAG_FEXTRA	 => 2,
	GZIP_FLAG_FNAME		 => 3,
	GZIP_FLAG_FCOMMENT	 => 4,
	# the rest are reserved
};
# compression level
use constant {
	GZIP_COMPRESSION_NORMAL	 => 0,
	GZIP_COMPRESSION_BEST	 => 2,
	GZIP_COMPRESSION_FAST	 => 4,
};
# operating systems
use constant {
	GZIP_OS_MSDOS		 => 0,
	GZIP_OS_AMIGA		 => 1,
	GZIP_OS_VMS		 => 2,
	GZIP_OS_UNIX		 => 3,
	GZIP_OS_VMCMS		 => 4,
	GZIP_OS_ATARI		 => 5,
	GZIP_OS_HPFS		 => 6,
	GZIP_OS_MACINTOSH	 => 7,
	GZIP_OS_ZSYSTEM		 => 8,
	GZIP_OS_CPM		 => 9,
	GZIP_OS_TOPS		 => 10,
	GZIP_OS_NTFS		 => 11,
	GZIP_OS_QDOS		 => 12,
	GZIP_OS_RISCOS		 => 13,
	GZIP_OS_VFAT             => 14,
	GZIP_OS_UNKNOWN		 => 255,
};

1
