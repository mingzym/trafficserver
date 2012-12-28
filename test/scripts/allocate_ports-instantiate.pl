#!/usr/bin/perl -w
#
#  allocate_ports-instantiate.pl
#  Author: stephane
#
#  Description:
#
#  DEFT framework generic instantiator to allocate ports.
#
#  (c) 2002 Inktomi Corporation.  All Rights Reserved.  Confidential.
#

use strict;

open(OUTPUT,">&=$ARGV[1]") || die "Couldn't open output: $!";

my %input_args;

while (my $tmp = <STDIN>) {
    print $tmp;
    if ($tmp =~ /^([^:]+):\s(.*)\n/) {
	$input_args{$1} = $2;
    }
}

my $ports = $input_args{"ports_avail"};
my $start_port;
my $end_port;

if ($ports =~ /^(\d+)-(\d+)/) {
    $start_port = $1;
    $end_port = $2;
} else {
    warn("ports_avail invalid\n");
}

my $config_blob = $input_args{"config_file"};

my $ports_used = 0;
my $port_binding;

if ($config_blob) {
    open(CONFIG_IN, "< $config_blob") || die "Couldn't open config file: $!";

    my $ports = <CONFIG_IN>;
    chomp($ports);

    print "ports: $ports\n";

    foreach my $port_name (split(/ /, $ports)) {
        $port_binding .= " $port_name " . ($start_port + $ports_used);
        $ports_used++;
    }

    close(CONFIG_IN);
}

print "ports_used:$ports_used\n";
print "port_binding: $port_binding\n\n";

print OUTPUT "ports_used:$ports_used\n";
print OUTPUT "port_binding: $port_binding\n";

exit(0);

