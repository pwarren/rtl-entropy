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

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.6)
project(rtl-entropy C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

if(NOT EXEC_INSTALL_DIR)
  set(EXEC_INSTALL_DIR bin)
endif()

# Set the version information here
set(VERSION_INFO_MAJOR_VERSION 0)
set(VERSION_INFO_MINOR_VERSION 1)
set(VERSION_INFO_PATCH_VERSION 1)
include(Version)

########################################################################
# Compiler specific setup
########################################################################

IF(CMAKE_CXX_COMPILER MATCHES ".*clang")
  SET(CMAKE_COMPILER_IS_CLANGXX 1)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Wextra)
    ADD_DEFINITIONS(-Wno-unused-parameter)
    ADD_DEFINITIONS(-Wsign-compare)
    #ADD_DEFINITIONS(-Wconversion)
    ADD_DEFINITIONS(-pedantic)
    #ADD_DEFINITIONS(-ansi)
    IF(NOT WIN32)
      #only export symbols that are declared to be part of the api (non dll platforms)
      ADD_DEFINITIONS(-fvisibility=hidden)
      ADD_DEFINITIONS(-fvisibility-inlines-hidden)
    ENDIF(NOT WIN32)
    ADD_DEFINITIONS(${OPENSSL_DEFINITIONS})
  ENDIF()

########################################################################
# Find build dependencies
########################################################################
find_package(LibRTLSDR)
#find_package(LibbladeRF)
find_package(OpenSSL)
find_package(LibCAP)

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
  ${OPENSSL_INCLUDE_DIRS}
  ${LibCAP_INCLUDE_DIR}
  ${LIBRTLSDR_INCLUDE_DIRS}
)

link_directories(
  ${OPENSSL_LIBRARIES}
  ${LibCAP_LIBRARY}
  ${LIBRTLSDR_LIBRARIES}
)

add_subdirectory(src)

########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)


########################################################################
# Create Pkg Config File
########################################################################
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix}/bin)
set(libdir \${prefix}/lib)
set(includedir \${prefix}/include)

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl-entropy.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/rtl-entropy.pc
@ONLY)



INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/rtl-entropy.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/pkgconfig
)

########################################################################
# Print Summary
########################################################################
MESSAGE(STATUS "Building for version: ${VERSION} / ${LIBVER}")
MESSAGE(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
