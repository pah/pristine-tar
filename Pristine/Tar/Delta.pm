#!/usr/bin/perl
# pristine-tar delta file library
# See delta-format.txt for details about the contents of delta files.
package Pristine::Tar::Delta;

use Pristine::Tar;
use warnings;
use strict;

# Checks if a field of a delta should be stored in the delta hash using
# a filename. (Normally the hash stores the whole field value, but
# using filenames makes sense for a few fields.)
my %delta_files=map { $_ => 1 } qw(manifest delta wrapper);
sub is_filename {
	my $field=shift;
	return $delta_files{$field};
}

sub handler {
	my $action=shift;
	my $type=shift;
	
	my $class="Pristine::Tar::Delta::$type";
	eval "use $class";
	if ($@) {
		error "unsupported delta file format $type";
	}
	$class->$action(@_);
}

# After the type of delta and the file to create (which can be "-"
# to send it to stdout), this takes a hashref containing the contents of
# the delta to write.
sub write {
	my $type=shift;
	my $deltafile=shift;
	my $delta=shift;

	my $tempdir=tempdir();

	my $stdout=0;
	if ($deltafile eq "-") {
		$stdout=1;
		$deltafile="$tempdir/tmpout";
	}
	
	handler('write', $type, $deltafile, $delta);

	if ($stdout) {
		doit("cat", $deltafile);
		unlink($deltafile);
	}

	return $delta;
}

# Returns a hashref of the contents of the delta.
sub read {
	my $type=shift;
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

	my $delta=handler('read', $type, $deltafile);
	
	unlink($deltafile) if $stdin;

	return $delta;
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
