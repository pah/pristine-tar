#!/usr/bin/perl
# pristine-tar delta file library
package Pristine::Tar::Delta;

use Pristine::Tar;
use warnings;
use strict;
use File::Basename;

# See delta-format.txt for details about the contents of delta files.
#
# Some of the delta contents are treated as files. Things not listed here
# are treated as fields with short values.
my %delta_files=map { $_ => 1 } qw(manifest delta wrapper);

# After the filename to create, this takes a hashref containing
# the contents of the delta file to create.
sub write {
	my $deltafile=shift;
	my $delta=shift;

	my $tempdir=tempdir();

	my $stdout=0;
	if ($deltafile eq "-") {
		$stdout=1;
		$deltafile="$tempdir/tmpout";
	}

	foreach my $field (keys %$delta) {
		if ($delta_files{$field}) {
			link($delta->{$field}, "$tempdir/$field") || die "link $tempdir/$field: $!";
		}
		else {
			open (my $out, ">", "$tempdir/$field") || die "$tempdir/$field: $!";
			print $out $delta->{$field}."\n";
			close $out;
		}
	}

	doit("tar", "czf", $deltafile, "-C", $tempdir, keys %$delta);

	if ($stdout) {
		doit("cat", $deltafile);
		unlink($deltafile);
	}

	return $delta;
}

# Returns a hashref of the contents of the delta.
sub read {
	my $deltafile=shift;
	
	my $tempdir=tempdir();

	my $stdin=0;
	if ($deltafile eq "-") {
		$deltafile="$tempdir/tmpin";
		open (my $out, ">", $deltafile) || die "$deltafile: $!";
		while (<STDIN>) {
			print $out $_;
		}
		close $out;
	}
	doit("tar", "xf", File::Spec->rel2abs($deltafile), "-C", $tempdir);
	unlink($deltafile) if $stdin;

	my %delta;
	foreach my $file (glob("$tempdir/*")) {
		if (-f $file) {
			my $field=basename($file);
			if ($delta_files{$field}) {
				$delta{$field}=$file;
			}
			else {
				open (my $in, "<", $file) || die "$file: $!";
				{
					local $/=undef;
					$delta{$field}=<$in>;
				}
				chomp $delta{$field};
				close $in;
			}
		}
	}
	# TODO read all files
	
	return \%delta;
}

# Checks the type, maxversion, minversion of a delta hashref.
# Checks that the delta contains all specified fields.
# Returns the hashref if it is ok.
sub assert {
	my $delta=shift;
	my %params=@_;

	if (! exists $delta->{version}) {
		error "delta lacks version";
	}
	if (defined $params{maxversion}) {
		if ($delta->{version} > $params{maxversion}) {
			error "delta is version ".$delta->{version}.", newer than maximum supported version $params{maxversion}";
		}
	}
	if (defined $params{minversion}) {
		if ($delta->{version} < $params{minversion}) {
			error "delta is version ".$delta->{version}.", older than minimum supported version $params{minversion}";
		}
	}

	if (! exists $delta->{type}) {
		error "delta lacks type";
	}
	if (defined $params{type}) {
		if ($delta->{type} ne $params{type}) {
			error "delta is for a ".$delta->{type}.", not a $params{type}";
		}
	}

	if ($params{fields}) {
		foreach my $key (@{$params{fields}}) {
			if (! exists $delta->{$key}) {
				error "delta lacks $key";
			}
		}
	}

	return $delta;
}

1
