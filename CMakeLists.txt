###########################################################################
#   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 #
#                                                                         #
#   This file is part of LuxRays.                                         #
#                                                                         #
#   LuxRays is free software; you can redistribute it and/or modify       #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 3 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   LuxRays is distributed in the hope that it will be useful,            #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
#                                                                         #
#   LuxRays website: http://www.luxrender.net                             #
###########################################################################

MESSAGE(STATUS "CMAKE VERSION DETECTED " ${CMAKE_VERSION})


###########################################################################
#
# Check and configure cmake
#
###########################################################################
# Fresh start
cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

#Remove the following when the version check is at least 2.8.4
SET(CMAKE_LEGACY_CYGWIN_WIN32 0)

project(LuxRays)

# Dont overconfigure
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limit configs" FORCE)


###########################################################################
#
# Include necessary submodules
#
###########################################################################

set(CMAKE_MODULE_PATH
	"${LuxRays_SOURCE_DIR}"
	"${LuxRays_SOURCE_DIR}/cmake"
	"${LuxRays_SOURCE_DIR}/cmake/Utils"
	"${LuxRays_SOURCE_DIR}/cmake/Packages"
	"${LuxRays_SOURCE_DIR}/cmake/SpecializedConfig"
)

INCLUDE(PlatformSpecific)
INCLUDE(Configuration)
INCLUDE(KernelPreprocess)

# Install CMake modules
#add_subdirectory(CMake)

SET(LuxRays_INCLUDE_DIR "${LuxRays_SOURCE_DIR}/include")
include_directories("${LuxRays_INCLUDE_DIR}")

# When using single configuration generators like make
# cmake does need to know which of the possible configurations
# to generate
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)
SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "assure config" FORCE) # makes sure type is shown in cmake gui

message(STATUS "Building mode: " ${CMAKE_BUILD_TYPE})

# Find dependencies
include(Dependencies)

if (NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required Boost files - Please check ${BOOST_SEARCH_PATH}")
endif()

if (NOT FreeImage_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required FreeImage files - Please check ${FREEIMAGE_SEARCH_PATH}")
endif()

if (NOT OPENCL_FOUND AND NOT LUXRAYS_DISABLE_OPENCL)
	MESSAGE(ERROR "--> Could not locate required OpenCL files, disabling OpenCL support - Please check ${OPENCL_SEARCH_PATH}")
	SET(LUXRAYS_DISABLE_OPENCL 1)
endif()

if (NOT OPENGL_FOUND AND NOT LUXRAYS_DISABLE_OPENCL)
	MESSAGE(ERROR "--> Could not locate required OpenGL files, disabling OpenCL support, disabling samples build")
	SET(LUXRAYS_DISABLE_OPENCL 1)
endif()

if (NOT GLEW_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required GLEW files, disabling samples build - Please check ${GLEW_SEARCH_PATH}")
endif()

if (NOT GLUT_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required GLUT files, disabling samples build - Please check ${GLUT_SEARCH_PATH}")
endif()

if (LUXRAYS_DISABLE_OPENCL)
	ADD_DEFINITIONS("-DLUXRAYS_DISABLE_OPENCL")
endif()

###########################################################################
#
# LuxRays
#
###########################################################################

# Add source files to LuxRays
add_subdirectory(src)


###########################################################################
#
# Samples
#
###########################################################################

add_subdirectory(samples/benchsimple)
if(OPENGL_FOUND AND GLUT_FOUND AND GLEW_FOUND)
	if(NOT LUXRAYS_DISABLE_OPENCL)
		if(BUILD_LUXMARK)
			message(STATUS "Building LuxMark too - this will require QT4")
			#add_subdirectory(samples/luxmark)
		endif(BUILD_LUXMARK)
	else(NOT LUXRAYS_DISABLE_OPENCL)
		if(BUILD_LUXMARK)
			message(WARNING "LuxMark requires OpenCL support, not building")
		endif(BUILD_LUXMARK)
	endif(NOT LUXRAYS_DISABLE_OPENCL)

	add_subdirectory(samples/smallluxgpu4)
endif(OPENGL_FOUND AND GLUT_FOUND AND GLEW_FOUND)


###########################################################################
#
# For non win32 we'll have to copy everything to a single dir
#
###########################################################################

INCLUDE(AssembleBinDirs)
