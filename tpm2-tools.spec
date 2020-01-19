Name: tpm2-tools
Version: 3.0.4
Release: 2%{?dist}
Summary: A TPM2.0 testing tool build upon TPM2.0-TSS

License: BSD
URL:     https://github.com/tpm2-software/tpm2-tools
Source0: https://github.com/tpm2-software/tpm2-tools/archive/%{version}.tar.gz#/%{name}-%{version}.tar.gz

# work around lack of pandoc in RHEL7
Patch0: add-man-pages.patch
# Deal with RHEL rpmbuilds not being from git
Patch1: autoconf-fixup.patch
Patch2: 0001-tpm2_create-Use-better-object-attributes-defaults-fo.patch

BuildRequires: gcc-c++
BuildRequires: libtool
BuildRequires: autoconf-archive
BuildRequires: pkgconfig(cmocka)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(openssl)
# tpm2-tss-devel provides sapi/tcti-device/tcti-socket
BuildRequires: pkgconfig(sapi)
BuildRequires: pkgconfig(tcti-device)
BuildRequires: pkgconfig(tcti-socket)
BuildRequires: pkgconfig(tcti-tabrmd)

# this package does not support big endian arch so far,
# and has been verified only on Intel platforms.
ExclusiveArch: %{ix86} x86_64

# tpm2-tools is heavily depending on TPM2.0-TSS project, matched tss is required
Requires: tpm2-tss%{?_isa} >= 1.3.0-1%{?dist}

# tpm2-tools project changed the install path for binaries and man page section
Obsoletes: tpm2-tools <= 2.1.0-2

%description
tpm2-tools is a batch of testing tools for tpm2.0. It is based on tpm2-tss.

%prep
%autosetup -p1 -n %{name}-%{version}
./bootstrap

%build
%configure --prefix=/usr --disable-static --disable-silent-rules
%make_build

%install
%make_install

%files
%doc README.md CHANGELOG.md
%license LICENSE
%{_bindir}/tpm2_*
%{_mandir}/man1/tpm2_*.1.gz

%changelog
* Thu Sep 06 2018 Jerry Snitselaar <jsnitsel@redhat.com> - 3.0.4-2
- tpm2_create: Use better object attributes defaults for authentication
resolves: rhbz#1627282

* Fri Jun 15 2018 Jerry Snitselaar <jsnitsel@redhat.com> - 3.0.4-1
- Rebase to 3.0.4 release
resolves: rhbz#1515108

* Wed Dec 13 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 3.0.1-1
- Rebase to 3.0.1 release
resolves: rhbz#1463100

* Wed Oct 18 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 2.1.0-2
- Fix potential memory leak
resolves: rhbz#1463100

* Wed Aug 30 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 2.1.0-1
- Rebase to 2.1.0 release
resolves: rhbz#1463100

* Mon May 15 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-7
- decide pcrs to read based off data returned from TPM2_GetCapability
resolves: rhbz#1449276

* Wed Apr 19 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-6
- check for null ptrs in RetrieveEndorsementCredentials

* Tue Apr 04 2017 Jerry Snitselaar <jsnitsel@redhat.com> - 1.1.0-5
- Remove epel dependencies
- Change tpm2-tss dependency to not be tied to 1 version
- Fix resource leak in InitSysContext
- Clean up HashEKPublicKey
- Add needed null checks to tpm2_getmanufec
- clean up resource leak in tpm2_getmanufec
- use strdup to get server address in tpm2_getmanufec
- change preparePcrSelections_g to void
- return on success in print_rc_tpm_error_code
- Update release version
resolves: rhbz#1275029 - Add tpm2.0-tools package

* Fri Jan 20 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-4
- Dependency check failed for Requires again, here to fix this
- Update release version and changelog

* Thu Jan 19 2017 Sun Yunying <yunying.sun@intel.com> - 1.1.0-3
- Change spec file permission to 644 to avoid rpmlint complain
- Update Requires to fix dependency check error reported in Bodhi
- Remove tpm2-tss-devel version in BuildRequires comment
- Update release version and changelog

* Wed Dec 21 2016 Sun Yunying <yunying.sun@intel.com> - 1.1.0-2
- Remove pkg_version to avoid dupliate use of version
- Remove redundant BuildRequires for autoconf/automake/pkgconfig
- Add comments for BuildRequires of sapi/tcti-device/tcti-socket
- Use ExclusiveArch instead of ExcludeArch
- Requires tpm2-tss version updated to 1.0-2
- Updated release version and changelog

* Fri Dec 2 2016 Sun Yunying <yunying.sun@intel.com> - 1.1.0-1
- Initial version of the package
