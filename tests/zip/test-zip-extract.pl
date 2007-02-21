#!/usr/bin/env perl
chomp($cwd = `pwd`);
$test_archive = "$cwd/data/test-extract.zip";

print "========================================";
print "========================================\n";
print "Target archive:\n";
print "$test_archive\n";

unlink "$cwd/data/extract/1.txt";

@args = ("./test-extract", "-d", "$cwd/data/extract", "-e", $test_archive);
system(@args) == 0
	or die "system @args failed: $?";

#die "A" unless -e "$cwd/data/extract/1.txt";

unlink $test_archive;

print "========================================";
print "========================================\n";
