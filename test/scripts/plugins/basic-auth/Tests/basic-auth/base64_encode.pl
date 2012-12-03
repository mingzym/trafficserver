#!/usr/local/bin/perl -w

use MIME::Base64;

print("enter string to encode: ");
$input = <STDIN>;
chomp ($input);

$encoded = encode_base64($input, '');

print ("encoded string (without the quotes): \"$encoded\"\n");
