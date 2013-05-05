#!/usr/bin/perl
# pristine-tar utility library

package Pristine::Tar;

use warnings;
use strict;
use File::Temp;
use Getopt::Long;
use IPC::Open2;
use Exporter q{import};

our @EXPORT = qw(error message debug vprint doit try_doit doit_redir
	tempdir dispatch comparefiles
	$verbose $debug $keep);

our $verbose=0;
our $debug=0;
our $keep=0;

sub progname {
	my $name=$0;
	$name=~s!^.*/(.+)$!$1!;
	return $name
}

sub error {
	die progname().": @_\n";
}

sub message {
	print STDERR progname().": @_\n";
}

sub debug {
	message(@_) if $debug;
}

sub vprint {
	message(@_) if $verbose;
}

sub doit {
	if (try_doit(@_) != 0) {
		error "command failed: @_";
	}
}

sub try_doit {
	vprint(@_);
	return system(@_)
}

sub doit_redir {
	no warnings 'once';
	my ($in, $out, @args) = @_;
	vprint(@args, "<", $in, ">", $out);
	open INFILE, "<", $in or die("Could not open '$in' for reading: $!\n");
	open OUTFILE, ">", $out or die("Could not open '$out' for reading: $!\n");
	my $pid = open2(">&OUTFILE", "<&INFILE", @args);
	waitpid $pid, 0;
	if ($? != 0) {
		error "command failed: @args";
	}
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

	my $command;
	Getopt::Long::Configure("bundling");
	if (! GetOptions(%options,
			"v|verbose!" => \$verbose,
			"d|debug!" => \$debug,
			"k|keep!" => \$keep) ||
	    ! @ARGV) {
	    	$command="usage";
	}
	else {
		$command=shift @ARGV;
	}

	my $i=$commands{$command};
	if (! defined $i) {
		error "Unknown subcommand \"$command\"";
	}

	# Check that the right number of args were passed by user.
	if (defined $i->[1] && @ARGV != $i->[1]) {
		$command="usage";
		$i=$commands{$command};
	}

	$i->[0]->(@ARGV);
}

sub comparefiles {
	my ($old, $new) = (shift, shift);
	system('cmp', '-s', $old, $new);

	if ($? == -1 || $? & 127) {
		die("Failed to execute cmp: $!\n");
	}

	return $? >> 8;
}

1
