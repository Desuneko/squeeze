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

open(ZIP, "unzip -lv -qq $test_file |")
	or die "Cant execute 'zip -lvqq $test_file'";

print "========================================";
print "========================================\n";
