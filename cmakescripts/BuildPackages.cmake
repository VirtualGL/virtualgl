# This file is included from the top-level CMakeLists.txt.  We just store it
# here to avoid cluttering up that file.


#
# Linux RPM and DEB
#

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

set(RPMARCH ${CPU_TYPE})
if(${CPU_TYPE} STREQUAL "x86_64")
	set(DEBARCH amd64)
else()
	set(DEBARCH ${CPU_TYPE})
endif()

string(TOLOWER ${CMAKE_PROJECT_NAME} CMAKE_PROJECT_NAME_LC)

configure_file(release/makerpm.in pkgscripts/makerpm)
configure_file(release/${CMAKE_PROJECT_NAME}.spec.in
	pkgscripts/${CMAKE_PROJECT_NAME}.spec @ONLY)

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

if(BITS EQUAL 64)
	set(INST_DEFS -DWIN64)
endif()

if(MSVC_IDE)
	set(INST_DEFS ${INST_DEFS} "-DBUILDDIR=${CMAKE_CFG_INTDIR}\\")
else()
	set(INST_DEFS ${INST_DEFS} "-DBUILDDIR=")
endif()

configure_file(release/@CMAKE_PROJECT_NAME@.nsi.in pkgscripts/@CMAKE_PROJECT_NAME@.nsi @ONLY)

if(MSVC_IDE)
	add_custom_target(installer
		COMMAND ${CMAKE_COMMAND} -E make_directory @CMAKE_BINARY_DIR@/${CMAKE_CFG_INTDIR}
		COMMAND makensis -nocd ${INST_DEFS} pkgscripts/@CMAKE_PROJECT_NAME@.nsi
		DEPENDS tcbench nettest wglspheres
		SOURCES pkgscripts/@CMAKE_PROJECT_NAME@.nsi)
else()
	add_custom_target(installer
		COMMAND makensis -nocd ${INST_DEFS} pkgscripts/@CMAKE_PROJECT_NAME@.nsi
		DEPENDS tcbench nettest wglspheres
		SOURCES pkgscripts/@CMAKE_PROJECT_NAME@.nsi)
endif()

endif() # WIN32


#
# Cygwin Package
#

if(CYGWIN)

set(VGL_DOCSYMLINK 0)
set(VGL_SYSPREFIX ${CMAKE_INSTALL_PREFIX})
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr"
	AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
	option(VGL_USESYSDIR "Package essential VirtualGL client components in /usr, not ${CMAKE_INSTALL_PREFIX}" ON)
	if(VGL_USESYSDIR)
		set(VGL_SYSPREFIX /usr)
	endif()
	set(VGL_DOCSYMLINK 1)
endif()

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
configure_file(release/Distribution.xml.in pkgscripts/Distribution.xml)
configure_file(release/uninstall.in pkgscripts/uninstall)
configure_file(release/uninstall.applescript.in pkgscripts/uninstall.applescript)

add_custom_target(dmg sh pkgscripts/makemacpkg
	SOURCES pkgscripts/makemacpkg)

add_custom_target(udmg sh pkgscripts/makemacpkg universal
  SOURCES pkgscripts/makemacpkg)

endif() # APPLE


#
# Generic
#

configure_file(release/makesrctarball.in pkgscripts/makesrctarball)

add_custom_target(srctarball sh pkgscripts/makesrctarball
	SOURCES pkgscripts/makesrctarball)

configure_file(release/makesrpm.in pkgscripts/makesrpm)

add_custom_target(srpm sh pkgscripts/makesrpm
	SOURCES pkgscripts/makesrpm)
