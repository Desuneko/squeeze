#!/usr/bin/env perl
chomp($cwd = `pwd`);
$test_archive = "$cwd/data/test-extract.zip";

print "========================================";
print "========================================\n";
print "Target archive:\n";
print "$test_archive\n";

@args = ("./test-add", "-n", $test_archive, "$cwd/data/3.txt");
system(@args) != 0
	or die "system @args should fail: $?";

unlink $test_file;

@args = ("./test-add", "-n", $test_archive, "$cwd/data/1.txt");
system(@args) == 0
	or die "system @args failed: $?";

print "========================================";
print "========================================\n";
