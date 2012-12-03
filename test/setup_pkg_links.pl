#!/usr/local/bin/perl
#
# setup_pkg_links.pl
#
#   Author          : Mike Chowla
#
#   Description:
#
#   $Id: setup_pkg_links.pl,v 1.2 2003-06-01 18:38:29 re1 Exp $
#
#   (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

use strict vars;

if (scalar(@ARGV) != 2) {
    print("Usage: setup_pkg_links.pl <real_pkg_dir> <link_link_dir>\n");
}

my $real_pkg_dir = $ARGV[0];
my $link_pkg_dir = $ARGV[1];
my %already_have = ();

opendir(LINK_DIR, $link_pkg_dir) || die "Could not read dir $link_pkg_dir : $!\n";

my $tmp;
while ($tmp = readdir(LINK_DIR)) {
    if ($tmp =~ /(^[^-]+)-([^-]+)-([^-]+)-([^-]+)\.\d\d\.\d\d\.\d\d\.\d\d.tgz$/) {
	# Looks like a pkg file
	my $have_pkg = $1 . "-" . $2;
	print "Adding have pkg $have_pkg\n";

	$already_have{$have_pkg} = $tmp;
    }
}

close (LINK_DIR);


opendir(SRC_DIR, $real_pkg_dir) || die "Could not read dir $real_pkg_dir : $!\n";

while ($tmp = readdir(SRC_DIR)) {
    if ($tmp =~ /(^[^-]+)-([^-]+)-([^-]+)-([^-]+)\.\d\d\.\d\d\.\d\d\.\d\d.tgz$/) {
	# Looks like a pkg file
	my $src_pkg = $1 . "-" . $2;

	if ($already_have{$src_pkg} ne $tmp) {
            if (defined $already_have{$src_pkg}) {
                my $old_fake = $link_pkg_dir . '/' . $already_have{$src_pkg};
                unlink($old_fake) || warn "Unlink of $old_fake failed : $!";
                print "Unlinked $old_fake\n";
            }
	    my $real = $real_pkg_dir . "/" . $tmp;
	    my $fake = $link_pkg_dir . "/" . $tmp;
	    symlink($real, $fake) || warn "Symlink of $tmp failed : $!";
	    print "Symlinked $tmp\n";
	} else {
	    print "Skipping $tmp since we already have it\n";
	}
    }
}

close (SRC_DIR);

exit(0);
