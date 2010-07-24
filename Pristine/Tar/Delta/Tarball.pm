#!/usr/bin/perl
# pristine-tar delta files formatted as tarballs
package Pristine::Tar::Delta::Tarball;

use Pristine::Tar;
use Pristine::Tar::Delta;
use File::Basename;
use warnings;
use strict;

sub write {
	my $class=shift;
	my $deltafile=shift;
	my $delta=shift;

	my $tempdir=tempdir();

	foreach my $field (keys %$delta) {
		if (Pristine::Tar::Delta::is_filename($field)) {
			link($delta->{$field}, "$tempdir/$field") || die "link $tempdir/$field: $!";
		}
		else {
			open (my $out, ">", "$tempdir/$field") || die "$tempdir/$field: $!";
			print $out $delta->{$field}."\n";
			close $out;
		}
	}

	doit("tar", "czf", $deltafile, "-C", $tempdir, keys %$delta);

	return $delta;
}

sub read {
	my $class=shift;
	my $deltafile=shift;
	
	my $tempdir=tempdir();
	doit("tar", "xf", File::Spec->rel2abs($deltafile), "-C", $tempdir);

	my %delta;
	foreach my $file (glob("$tempdir/*")) {
		if (-f $file) {
			my $field=basename($file);
			if (Pristine::Tar::Delta::is_filename($field)) {
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
	
	return \%delta;
}

1
