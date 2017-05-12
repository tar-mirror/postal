#!/usr/bin/perl

# use:
# mkzones.pl 100 a%s.example.com 10.254.0 10.253.0.7
# the above command creates zones a0.example.com to a99.example.com with an
# A record for the mail server having the IP address 10.254.0.X where X is
# a number from 1 to 254 and an NS record
# with the IP address 10.1.2.4
#
# then put the following in your /etc/named.conf
#include "/etc/named.conf.postal";
#
# the file "users" in the current directory will have a sample user list for
# postal
#


my $inclfile = "/etc/named.conf.postal";
open(INCLUDE, ">$inclfile") or die "Can't create $inclfile";
open(USERS, ">users") or die "Can't create users";
my $zonedir = "/var/named/data";
for(my $i = 0; $i < $ARGV[0]; $i++)
{
  my $zonename = sprintf($ARGV[1], $i);
  my $filename = "$zonedir/$zonename";
  open(ZONE, ">$filename") or die "Can't create $filename";
  print INCLUDE "zone \"$zonename\" {\n  type master;\n  file \"$filename\";\n};\n\n";
  print ZONE "\$ORIGIN	$zonename.\n\$TTL 86400\n\@	SOA	localhost.	root.localhost. (\n";
# serial refresh retry expire ttl
  print ZONE "	2006092501 36000 3600 604800 86400 )\n";
  print ZONE "	IN NS ns.$zonename.\n";
  print ZONE "	IN MX 10 mail.$zonename.\n";
  my $final = $i % 254 + 1;
  print ZONE "mail	IN A $ARGV[2].$final\n";
  print ZONE "ns	IN A $ARGV[3]\n";
  close(ZONE);
  print USERS "user$final\@$zonename\n";
}
close(INCLUDE);
close(USERS);
