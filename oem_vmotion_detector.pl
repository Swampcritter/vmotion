#!/usr/bin/perl
#
# OEM vMotion Dectector
#
# This front-end script is for OEM to kickoff the VM Guest SDK 'Session ID'
# executable to check to see if a RHEL VM has been vMotion'd or not.
#

$ENV{PATH} = "/bin:/usr/local/bin:/usr/sbin:/usr/bin"; 

system("/usr/local/bin/vmguestsessionid");

