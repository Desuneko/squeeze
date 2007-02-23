#!/usr/bin/env perl
chomp($cwd = `pwd`);
my $test_archive = ["$cwd/data/test-archive.tar", "$cwd/data/test-archive.tar.gz", "$cwd/data/test-archive.tar.bz2"];

print "========================================";
print "========================================\n";

#for(i = 0; i < 
for $i (0 .. 2) {
	print "Target archive:\n";
	print "$test_archive->[1]\n";
	@args = ("./test-add", "-n", $test_archive->[$i], "$cwd/data/3.txt");
	system(@args) != 0
		or die "system @args should fail: $?";

	unlink $test_file;

	@args = ("./test-add", "-n", $test_archive->[$i], "$cwd/data/1.txt");
	system(@args) == 0
		or die "system @args failed: $?";

	@args = ("./test-add", "-a", $test_archive->[$i], "$cwd/data/2.txt");
	system(@args) == 0
		or die "system @args failed: $?";
}

print "========================================";
print "========================================\n";

