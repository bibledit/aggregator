# Copyright (©) 2026-2026 Teus Benschop.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


# Set the minimum CMake version required.
cmake_minimum_required(VERSION 3.28)


# Print info.
message(STATUS "Processing ${CMAKE_CURRENT_SOURCE_DIR}")


# Start the project.
project(aggregator
	VERSION 0.1.0
	LANGUAGES C CXX
)
message(STATUS "Aggregator version ${aggregator_VERSION}")


set(COMPILER_GCC OFF)
set(COMPILER_CLANG OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(COMPILER_GCC ON)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang") # Clang, AppleClang, ...
	set(COMPILER_CLANG ON)
endif()


# Handle the install prefix.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "/usr")
endif()
option(CMAKE_INSTALL_PREFIX "Install prefix" "/usr")
message(STATUS "Install prefix is ${CMAKE_INSTALL_PREFIX} (change: $cmake -DCMAKE_INSTALL_PREFIX=path)")
# Set it during install: $ cmake --install . --prefix "/path"


# Compiler to emit info for the debugger.
add_compile_options(-g)


# Enable compiler warnings.
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-pragmas)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wshadow>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Warray-bounds>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wcast-align>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wcast-qual>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wconversion>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wctor-dtor-privacy>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wdelete-non-virtual-dtor>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Weffc++>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wextra-semi>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wfloat-equal>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wformat=2>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wmissing-include-dirs>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wmissing-noreturn>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wnull-dereference>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wredundant-decls>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wsign-promo>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wsuggest-override>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wswitch-default>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wswitch-enum>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wundef>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wuninitialized>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wunused-but-set-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wunused-macros>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wunused-parameter>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wvla>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wzero-as-null-pointer-constant>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wmismatched-tags>)
if (COMPILER_GCC)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wcatch-value=2>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wconditionally-supported>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wduplicated-branches>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wduplicated-cond>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wformat-overflow=2>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wlogical-op>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wnoexcept>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-non-template-friend>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wrestrict>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wstrict-null-sentinel>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wuseless-cast>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wcomma-subscript>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wredundant-tags>)
	add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wvolatile>)
endif()
if (COMPILER_CLANG)
endif()

# Compilers on some systems may treat warnings as errors.
# Pass a flag to the compiler to treat a warning not as an error.
add_compile_options(-fno-var-tracking -Wno-error)


add_link_options(-Wall -lpthread -L. -g -rdynamic -ldl)


# Finding executables.
find_program(GZIP gzip REQUIRED)
find_program(GUNZIP gunzip REQUIRED)
find_program(TAR tar REQUIRED)
find_program(ZIP zip REQUIRED)
find_program(UNZIP unzip REQUIRED)
find_program(INSTALLMGR installmgr REQUIRED)
find_program(DIATHEKE diatheke REQUIRED)


# The C++ standard to use, and it is mandatory.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)


# Build the library.
add_library(library STATIC)
add_library(aggregator::library ALIAS library)
target_sources(library
	PRIVATE
		arguments.cpp
		curl.cpp
		logger.cpp
		sword/sword.cpp
		sword/store.cpp
		utilities.cpp
	PUBLIC
	FILE_SET HEADERS
	FILES
		arguments.h
		curl.h
		logger.h
		sword/sword.h
		sword/store.h
		utilities.h
)


# The aggregator executable.
add_executable(aggregator
	main.cpp
)
target_link_libraries(aggregator PRIVATE library)


# The unit tests executable
add_executable(unittest
	unittests/main.cpp
	unittests/sword.cpp
)
target_link_libraries(unittest PRIVATE library)

# The compile definitions.
add_compile_definitions(
	VERSION="${aggregator_VERSION}"
	BUILD_DIR="${CMAKE_BINARY_DIR}"
	SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)


# The required system libraries.
find_package(CURL 8.0.0 REQUIRED)
find_package(GTest 1.10.0 REQUIRED)
find_package(nlohmann_json 3.10.0 REQUIRED)
target_link_libraries(library PRIVATE
	nlohmann_json::nlohmann_json # Header-only, needed at compile time, not link time.
)
target_link_libraries(aggregator PRIVATE
	CURL::libcurl
)
target_link_libraries(unittest PRIVATE
	CURL::libcurl
	GTest::gtest
)



# Installation.
#install(TARGETS aggregator DESTINATION bin)


# Create tarball with target "dist"
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
add_custom_target(dist
	COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD --format=tar.gz ":(exclude).idea" ":(exclude).github" ":(exclude).gitignore" ":(exclude)cloud-macos.entitlements" > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	VERBATIM
)
