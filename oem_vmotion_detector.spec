Summary: VMware vMotion Detector for Oracle OEM
Name: oem_vmotion_detector
Version: 1.0
Release: 1
Group: System Tools
License: LGPL
URL: http://www.vmware.com/support/developer/guest-sdk/
Packager: Michael Worsham <mworsham@scires.com>
BuildArch: x86_64

%description
This bundled application includes a modified C executable that
will detect when a RHEL VM has been vMotion'd upon a standalone
or DRS cluster host environment. The executable is supported
through a Perl-based front end, thus allowing Oracle OEM to utilize
the script and then kick off the executable to obtain the end result
code and messages paired with it as well. The VMware Guest SDK was
utilized for the compilation of the actual program.

%install

%files
/usr/local/bin/oem_vmotion_detector.pl
/usr/local/bin/vmguestsessionid

%changelog
* Tue Aug 9 2011 Michael Worsham <mworsham@scires.com>
- Initial creation of vMotion detector build

