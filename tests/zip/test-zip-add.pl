#!/usr/bin/env perl
my $mime_type = "application/x-zip";
open(ZIP, "./test-add -m $mime_type |");

$aaa = <ZIP>;
while($aaa)
{
	print $aaa;
	$aaa = <ZIP>;
}
