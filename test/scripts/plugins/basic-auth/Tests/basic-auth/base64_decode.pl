#!/usr/local/bin/perl -w

use MIME::Base64;

print("enter string to decode\n");
$input = <STDIN>;
chomp ($input);

$decoded = decode_base64($input);

print ("decoded string (without the quotes): \"$decoded\"\n");
