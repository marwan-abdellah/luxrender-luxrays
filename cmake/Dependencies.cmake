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

include(FindPkgMacros)
getenv_path(LuxRays_DEPENDENCIES_DIR)

#######################################################################
# Core dependencies
#######################################################################

# Find threading library
FIND_PACKAGE(Threads REQUIRED)

# Find FreeImage
find_package(FreeImage)

if (FreeImage_FOUND)
	include_directories(SYSTEM ${FreeImage_INCLUDE_DIRS})
endif ()

# Find Boost
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(BOOST_ROOT                  "${BOOST_SEARCH_PATH}")
#set(Boost_DEBUG                 ON)
set(Boost_MINIMUM_VERSION       "1.44.0")

set(Boost_ADDITIONAL_VERSIONS "1.47.0" "1.46.1" "1.46" "1.46.0" "1.45" "1.45.0" "1.44" "1.44.0")

set(LUXRAYS_BOOST_COMPONENTS thread filesystem system)
find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${LUXRAYS_BOOST_COMPONENTS})
if (NOT Boost_FOUND)
		# Try again with the other type of libs
		if(Boost_USE_STATIC_LIBS)
				set(Boost_USE_STATIC_LIBS)
		else()
				set(Boost_USE_STATIC_LIBS OFF)
		endif()
		find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${LUXRAYS_BOOST_COMPONENTS})
endif()

if (Boost_FOUND)
	include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
	link_directories(${Boost_LIBRARY_DIRS})

	# File system library
	find_library(BOOST_FILESYSTEM_LIB NAMES boost_filesystem-mt
		HINTS ${BOOST_LIB_DIR}
		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
		)

	# System library
	find_library(BOOST_SYSTEM_LIB NAMES boost_system-mt
		HINTS ${BOOST_LIB_DIR}
		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
		)

	# Thread library
	find_library(BOOST_THREAD_LIB NAMES boost_thread-mt
		HINTS ${BOOST_LIB_DIR}
		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
		)

	include(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(BOOST DEFAULT_MSG
					BOOST_TIMER_LIB
					BOOST_FILESYSTEM_LIB
					BOOST_SYSTEM_LIB
					BOOST_THREAD_LIB
					BOOST_INC_DIR
	)

	# Include directories
	include_directories(${BOOST_INC_DIR})

	# Aggregate all the libraries in BOOST_LIBS
	SET(BOOST_LIBS
					${BOOST_FILESYSTEM_LIB}
					${BOOST_SYSTEM_LIB}
					${BOOST_THREAD_LIB})

	# Link against the boost libraries
	link_librarie(${BOOST_LIBS})

	# Don't use old boost versions interfaces
	ADD_DEFINITIONS(-DBOOST_FILESYSTEM_NO_DEPRECATED)
endif ()


find_package(OpenGL)

if (OPENGL_FOUND)
	include_directories(SYSTEM ${OPENGL_INCLUDE_PATH})
	link_libraries(${OPENGL_LIBRARY})
endif()

set(GLEW_ROOT                  "${GLEW_SEARCH_PATH}")
if(NOT APPLE)
	find_package(GLEW)
endif()

# GLEW
if (GLEW_FOUND)
	include_directories(SYSTEM ${GLEW_INCLUDE_PATH})
endif ()


set(GLUT_ROOT                  "${GLUT_SEARCH_PATH}")
find_package(GLUT)

# GLUT
if (GLUT_FOUND)
	include_directories(SYSTEM ${GLUT_INCLUDE_PATH})
endif ()

set(OPENCL_ROOT                  "${OPENCL_SEARCH_PATH}")
find_package(OpenCL)
# OpenCL
if (OPENCL_FOUND)
	include_directories(SYSTEM ${OPENCL_INCLUDE_DIR} ${OPENCL_C_INCLUDE_DIR})
endif ()
