Name:           qwalkingpad2
Version:        0.1
Release:        1%{?dist}
Summary:        Desktop Application for the Kingsmith WalkingPad

Group:          Applications/System
License:        GPL
URL:            https://github.com/DorianRudolph/QWalkingPad
#Source0:        https://github.com/DorianRudolph/QWalkingPad/archive/refs/heads/master.zip

BuildRequires:  cmake gcc gcc-c++ qt6-qtbase-devel qt6-qtconnectivity-devel
Requires:       qt6-qtbase qt6-qtconnectivity

%description
Qt6-based desktop Application for the Kingsmith WalkingPad, controls speed using BLE.

%prep
#git submodule init
#git submodule update
git clone --recursive %{url}.git

%build
cd QWalkingPad && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j

%install
tree
install -m 0755 -d %{buildroot}%{_exec_prefix}/bin/
install -m 0755 -d %{buildroot}%{_datarootdir}/applications/
install QWalkingPad/build/qwalkinkpad2 -m 0755 %{buildroot}%{_exec_prefix}/bin/qwalkingpad2
install QWalkingPad/qwalkingpad2.desktop -m 0644 %{buildroot}%{_datarootdir}/applications/qwalkingpad2.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%files
%{_exec_prefix}/bin/qwalkingpad2
%{_datarootdir}/applications/qwalkingpad2.desktop

%changelog
* Thu Aug 20 2026 Christian Stankowic <info@cstan.io> - 0.0.1
- First release
