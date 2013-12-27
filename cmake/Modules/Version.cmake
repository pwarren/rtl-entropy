# rtl-entropy, turns your Realtek RTL2832 based DVB dongle into a
# high quality entropy source.
#
# Copyright (C) 2013 by Paul Warren <pwarren@pwarren.id.au>
#
# Parts taken from:
#  - rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
#  - http://openfortress.org/cryptodoc/random/noise-filter.c
#      by Rick van Rein <rick@openfortress.nl>
#  - snd-egd Copyright (C) 2008-2010 Nicholas J. Kain <nicholas aatt kain.us>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.

if(DEFINED __INCLUDED_VERSION_CMAKE)
    return()
endif()
set(__INCLUDED_VERSION_CMAKE TRUE)

# VERSION_INFO_* variables must be provided by user
set(MAJOR_VERSION ${VERSION_INFO_MAJOR_VERSION})
set(MINOR_VERSION ${VERSION_INFO_MINOR_VERSION})
set(PATCH_VERSION ${VERSION_INFO_PATCH_VERSION})

########################################################################
# Extract the version string from git describe.
########################################################################
find_package(Git QUIET)

if(GIT_FOUND)
    message(STATUS "Extracting version information from git describe...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=4 --long
        OUTPUT_VARIABLE GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    set(GIT_DESCRIBE "v${MAJOR_VERSION}.${MINOR_VERSION}.x-xxx-xunknown")
endif()

########################################################################
# Use the logic below to set the version constants
########################################################################
if("${PATCH_VERSION}" STREQUAL "git")
    # VERSION: 3.6git-xxx-gxxxxxxxx
    # LIBVER:  3.6git
    set(VERSION "${GIT_DESCRIBE}")
    set(LIBVER  "${MAJOR_VERSION}.${MINOR_VERSION}${PATCH_VERSION}")
else()
    # This is a numbered release.
    # VERSION: 3.6.1
    # LIBVER:  3.6.1
    set(VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
    set(LIBVER "${VERSION}")
endif()
