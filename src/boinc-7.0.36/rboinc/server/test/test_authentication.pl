#!/usr/bin/perl -w

# $Id: test_authentication.pl 24510 2011-11-03 02:35:04Z romw $

# An authentication test script


require "./boinc_authentication.pl";

my $project_url='http://xxxx.org/LUNA';
my $email='yyyy@gmail.com';
my $password='zzzzz';


print lookup_email($project_url,$email);
print "\n\n";

print lookup_email_password($project_url,$email,$password);
print "\n\n";

