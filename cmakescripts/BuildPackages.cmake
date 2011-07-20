# This file is included from the top-level CMakeLists.txt.  We just store it
# here to avoid cluttering up that file.


#
# Linux RPM and DEB
#

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

if(64BIT)
	set(RPMARCH x86_64)
	set(DEBARCH amd64)
else()
	set(RPMARCH i386)
	set(DEBARCH i386)
endif()

set(VGL_DOCSYMLINK 0)
set(VGL_SYSPREFIX ${CMAKE_INSTALL_PREFIX})
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr"
	AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
	option(VGL_USESYSDIR "Package VirtualGL server and client executables in /usr, not ${CMAKE_INSTALL_PREFIX}" ON)
	if(VGL_USESYSDIR)
		set(VGL_SYSPREFIX /usr)
	endif()
	set(VGL_DOCSYMLINK 1)
endif()	

configure_file(release/makerpm.in pkgscripts/makerpm)
configure_file(release/VirtualGL.spec.in pkgscripts/VirtualGL.spec @ONLY)

add_custom_target(rpm sh pkgscripts/makerpm
	SOURCES pkgscripts/makerpm)

configure_file(release/makedpkg.in pkgscripts/makedpkg)
configure_file(release/deb-control.in pkgscripts/deb-control)

add_custom_target(deb sh pkgscripts/makedpkg
	SOURCES pkgscripts/makedpkg)

endif() # Linux


#
# Windows installer (NullSoft Installer)
#

if(WIN32)

if(64BIT)
  set(INST_DEFS -DWIN64)
endif()

if(MSVC_IDE)
  set(INST_DEFS ${INST_DEFS} "-DBUILDDIR=${CMAKE_CFG_INTDIR}\\")
else()
  set(INST_DEFS ${INST_DEFS} "-DBUILDDIR=")
endif()

configure_file(release/VirtualGL.nsi.in pkgscripts/VirtualGL.nsi @ONLY)

add_custom_target(installer
	makensis -nocd ${INST_DEFS} pkgscripts/VirtualGL.nsi
	DEPENDS vglclient tcbench nettest putty plink
	SOURCES pkgscripts/VirtualGL.nsi)

endif() # WIN32


#
# Cygwin Package
#

if(CYGWIN)

configure_file(release/makecygwinpkg.in pkgscripts/makecygwinpkg)

add_custom_target(cygwinpkg sh pkgscripts/makecygwinpkg)

endif() # CYGWIN


#
# Mac DMG
#

if(APPLE)

set(DEFAULT_VGL_32BIT_BUILD ${CMAKE_SOURCE_DIR}/osxx86)
set(VGL_32BIT_BUILD ${DEFAULT_VGL_32BIT_BUILD} CACHE PATH
  "Directory containing 32-bit OS X build to include in universal binaries (default: ${DEFAULT_VGL_32BIT_BUILD})")

string(REGEX REPLACE "/" ":" VGL_MACPREFIX ${CMAKE_INSTALL_PREFIX})
string(REGEX REPLACE "^:" "" VGL_MACPREFIX ${VGL_MACPREFIX})

configure_file(release/makemacpkg.in pkgscripts/makemacpkg)
configure_file(release/Info.plist.in pkgscripts/Info.plist)
configure_file(release/Description.plist.in pkgscripts/Description.plist)
configure_file(release/uninstall.in pkgscripts/uninstall)
configure_file(release/uninstall.applescript.in pkgscripts/uninstall.applescript)

add_custom_target(dmg sh pkgscripts/makemacpkg
	SOURCES pkgscripts/makemacpkg)

add_custom_target(udmg sh pkgscripts/makemacpkg universal
  SOURCES pkgscripts/makemacpkg)

endif() # APPLE


#
# Solaris package
#

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "sparc")
	set(DEFAULT_VGL_32BIT_BUILD ${CMAKE_SOURCE_DIR}/solaris)
	if(64BIT)
		set(PKGARCH sparcv9)
	else()
		set(PKGARCH sparc)
	endif()
else()
	set(DEFAULT_VGL_32BIT_BUILD ${CMAKE_SOURCE_DIR}/solx86)
	if(64BIT)
		set(PKGARCH amd64)
	else()
		set(PKGARCH i386)
	endif()
endif()
set(VGL_32BIT_BUILD ${DEFAULT_VGL_32BIT_BUILD} CACHE PATH
  "Directory containing 32-bit Solaris build to include in combined package (default: ${DEFAULT_VGL_32BIT_BUILD})")

configure_file(release/makesolarispkg.in pkgscripts/makesolarispkg)
configure_file(release/pkginfo.in pkgscripts/pkginfo)

add_custom_target(solarispkg sh pkgscripts/makesolarispkg
	SOURCES pkgscripts/makesolarispkg)

add_custom_target(csolarispkg sh pkgscripts/makesolarispkg combined
	SOURCES pkgscripts/makesolarispkg)

endif() # SunOS

