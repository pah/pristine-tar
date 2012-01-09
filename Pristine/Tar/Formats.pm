#!/usr/bin/perl
# pristine-tar file format parsing

package Pristine::Tar::Formats;

use warnings;
use strict;
use Exporter q{import};
our @EXPORT=qw{is_gz is_bz2 is_xz %fconstants};

our %fconstants=(
	# magic identification
	GZIP_ID1 => 0x1F,
	GZIP_ID2 => 0x8B,
	BZIP2_ID1 => 0x42,
	BZIP2_ID2 => 0x5a,
	XZ_ID1 => 0xFD,
	XZ_ID2 => 0x37,
	XZ_ID3 => 0x7A,
	XZ_ID4 => 0x58,
	XZ_ID5 => 0x5A,
	XZ_ID6 => 0x00,

	# compression methods
	# 0x00-0x07 are reserved
	GZIP_METHOD_DEFLATE => 0x08,
	# 'h' for Bzip2 ('H'uffman coding)
	BZIP2_METHOD_HUFFMAN => 0x68,

	# flags
	GZIP_FLAG_FTEXT => 0,
	GZIP_FLAG_FHCRC => 1,
	GZIP_FLAG_FEXTRA => 2,
	GZIP_FLAG_FNAME => 3,
	GZIP_FLAG_FCOMMENT => 4,
	# the rest are reserved

	# compression level
	GZIP_COMPRESSION_NORMAL => 0,
	GZIP_COMPRESSION_BEST => 2,
	GZIP_COMPRESSION_FAST => 4,

	# operating systems
	GZIP_OS_MSDOS => 0,
	GZIP_OS_AMIGA => 1,
	GZIP_OS_VMS => 2,
	GZIP_OS_UNIX => 3,
	GZIP_OS_VMCMS => 4,
	GZIP_OS_ATARI => 5,
	GZIP_OS_HPFS => 6,
	GZIP_OS_MACINTOSH => 7,
	GZIP_OS_ZSYSTEM => 8,
	GZIP_OS_CPM => 9,
	GZIP_OS_TOPS => 10,
	GZIP_OS_NTFS => 11,
	GZIP_OS_QDOS => 12,
	GZIP_OS_RISCOS => 13,
	GZIP_OS_VFAT => 14,
	GZIP_OS_UNKNOWN => 255,
);

sub magic {
	my $file=shift;

	open (my $in, "<", $file) || die "$file: $!";
	my $count=$#_+1;
	my ($chars, @bits);
	my $ret = (
		read($in, $chars, $count) == $count &&
		(@bits = unpack(("C" x $count), $chars)) &&
		(! grep { $bits[$_] != $_[$_] } (0..$count-1))
    	);
	close $in;
	return $ret;
}

sub is_gz {
	magic(shift, $fconstants{GZIP_ID1}, $fconstants{GZIP_ID2},
		$fconstants{GZIP_METHOD_DEFLATE});
}

sub is_bz2 {
	magic(shift, $fconstants{BZIP2_ID1}, $fconstants{BZIP2_ID2},
		$fconstants{BZIP2_METHOD_HUFFMAN});
}

sub is_xz {
	magic(shift, $fconstants{XZ_ID1}, $fconstants{XZ_ID2},
		$fconstants{XZ_ID3}, $fconstants{XZ_ID4},
		$fconstants{XZ_ID5}, $fconstants{XZ_ID6});
}

1
