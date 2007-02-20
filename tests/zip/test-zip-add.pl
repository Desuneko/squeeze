#!/usr/bin/env perl
chomp($cwd = `pwd`);
$test_file = "$cwd/data/test.zip";

print "========================================";
print "========================================\n";
print "Target archive:\n";
print "$test_file\n";

@args = ("./test-add", "-n", $test_file, "$cwd/data/1.txt");
system(@args) == 0
	or die "system @args failed: $?";

unlink $test_file;

@args = ("./test-add", "-n", $test_file, "$cwd/data/2.txt");
system(@args) != 0
	or die "system @args should fail: $?";

unlink $test_file;

print "========================================";
print "========================================\n";
