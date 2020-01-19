Name: tpm2-tools
Version: 1.1.0 
Release: 7%{?dist}
Summary: A TPM2.0 testing tool build upon TPM2.0-TSS

%global pkg_prefix tpm2.0-tools

License: BSD
URL:     https://github.com/01org/tpm2.0-tools
Source0: https://github.com/01org/tpm2.0-tools/archive/v%{version}.tar.gz#/%{pkg_prefix}-%{version}.tar.gz
# RHEL only. code no longer exists upstream
Patch0000: fix-resource-leak-InitSysContext.patch
# RHEL only. Upstream commit 2b6bb441 contains this and more.
# Added code to clean up hash malloc in err paths
Patch0001: HashEKPublicKey-cleanup.patch
# Submitted upstream. https://github.com/01org/tpm2.0-tools/pull/272
# Slightly different for RHEL due to code differences
Patch0002: tpm2_getmanuc-null-check.patch
# Fix is part of upstream commit 2b6bb441.
Patch0003: tpm2_getmanufec-leak-clean.patch
# Similar to part of upstream commit 2b6bb441.
Patch0004: ekservaddr.patch
# RHEL only. code completely changed upstream
Patch0005: void-return-listpcrs.patch
# Upstream commit 778bd1a0a1b5
Patch0006: ret-on-success-rc-decode.patch
# Based on part of upstream commit 2b6bb441.
Patch0007: tpm2-getmanufec-null-ptr-checks.patch
# similar fix submitted upstream https://github.com/01org/tpm2.0-tools/pull/284
Patch0008: tpm2-listpcrs-select.patch

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: libtool
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(openssl)
# tpm2-tss-devel provides sapi/tcti-device/tcti-socket
BuildRequires: pkgconfig(sapi)
BuildRequires: pkgconfig(tcti-device)
BuildRequires: pkgconfig(tcti-socket)

# this package does not support big endian arch so far,
# and has been verified only on Intel platforms.
ExclusiveArch: %{ix86} x86_64

# tpm2-tools is heavily depending on TPM2.0-TSS project, matched tss is required
Requires: tpm2-tss%{?_isa} >= 1.0-2%{?dist} 

%description
tpm2-tools is a batch of testing tools for tpm2.0. It is based on tpm2-tss.

%prep
%autosetup -p1 -n %{pkg_prefix}-%{version}
./bootstrap

%build
%configure --prefix=/usr --disable-static --disable-silent-rules
%make_build

%install
%make_install

%files
%doc README.md CHANGELOG 
%license LICENSE
%{_sbindir}/tpm2_*

%changelog
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
