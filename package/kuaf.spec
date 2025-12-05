%define _topdir %(echo $PWD)/..
%define kuaf_root_path %{_topdir}/../..
%define name %(xmllint --xpath "//KUAF_INFO_SOFTWARE_NAME/text()" %{kuaf_root_path}/dependence.xml)
%define version %(xmllint --xpath "//KUAF_INFO_VERSION/text()" %{kuaf_root_path}/dependence.xml)
%define cpulimit %(xmllint --xpath "//KUAF_INFO_CPU_LIMIT/text()" %{kuaf_root_path}/dependence.xml)
%define kuafbuilddir %{_builddir}/%{name}-%{version}
%define kuafinstalldir /usr/local


autoreq: no

Name:           %{name}
Version:        %{version}
Release:        1%{?dist}
Summary:        kuaf library

License:        commercial
Source0:        %{name}-%{version}.tar.gz

%description
kuaf interface functions

%global debug_package %{nil}

%prep
%setup -c

%build
gccVer=4.8.5
gccCurVer=$(gcc -dumpversion)
if test "$(echo "${gccCurVer} ${gccVer}" | tr " " "\n" | sort -V | head -n 1)" != "${gccVer}"; then
    echo "gcc >=4.8.5 is need"
    exit 1
fi


if [ "$(cmake -version | grep version | cut -d ' ' -f 3)" \< "3.16.3" ]; then
echo "cmake >= 3.16.3 is need"
exit 1
fi

cd %{kuafbuilddir}
sh build.sh all

%install
install -d -m 755 %buildroot%{kuafinstalldir}/kuaf/include
install -d -m 755 %buildroot%{kuafinstalldir}/kuaf/include/comp
install -d -m 755 %buildroot%{kuafinstalldir}/kuaf/include/crypto
install -d -m 755 %buildroot%{kuafinstalldir}/kuaf/lib
install -d -m 755 %buildroot/etc/kuaf
cp -av %{kuafinstalldir}/kuaf/include/* %buildroot%{kuafinstalldir}/kuaf/include
cp -rf %{kuafinstalldir}/kuaf/lib/libz.* %buildroot%{kuafinstalldir}/kuaf/lib
cp -rf %{kuafinstalldir}/kuaf/lib/libkuaf.* %buildroot%{kuafinstalldir}/kuaf/lib
cp -rf /etc/kuaf/* %buildroot/etc/kuaf

%pre
implementer=$(cat /proc/cpuinfo | grep "CPU implementer" | awk 'NR==1{printf $4}')
if [ "Y" == "%{cpulimit}" ] && [ "${implementer}" != "0x48" ]; then
    echo "Only installed on kunpeng 920 series"
    exit 1
fi

%post
sed -i '$a\export C_INCLUDE_PATH=/usr/local/kuaf/include:$C_INCLUDE_PATH' /etc/profile
sed -i '$a\export CPLUS_INCLUDE_PATH=/usr/local/kuaf/include:$CPLUS_INCLUDE_PATH' /etc/profile
sed -i '$a\export PYTHONPATH=/usr/local/kuaf/include/:$PYTHONPATH' /etc/profile
source /etc/profile

%postun
sed -i '/\/usr\/local\/kuaf\/include/d' /etc/profile
sed -i '/\/usr\/local\/kuaf\/lib/d' /etc/profile
source /etc/profile
if [ -d "%{kuafinstalldir}/kuaf" ]; then
    rm -rf %{kuafinstalldir}/kuaf
fi

%files
%defattr(-,root,root)
%{kuafinstalldir}/kuaf
/etc/kuaf

%changelog