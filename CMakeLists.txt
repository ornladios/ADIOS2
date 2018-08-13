#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.6)

# Fail immediately if not using an out-of-source build
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported.  Please create a build directory "
    "separate from the source directory")
endif()

project(ADIOS2 VERSION 2.2.0)
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

#------------------------------------------------------------------------------#
# Some boilerplate to setup nice output directories
#------------------------------------------------------------------------------#
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/adios2
  CACHE STRING "Installation CMake subdirectory")
mark_as_advanced(CMAKE_INSTALL_BINDIR)
mark_as_advanced(CMAKE_INSTALL_LIBDIR)
mark_as_advanced(CMAKE_INSTALL_INCLUDEDIR)
mark_as_advanced(CMAKE_INSTALL_CMAKEDIR)

list(INSERT CMAKE_MODULE_PATH 0 "${PROJECT_SOURCE_DIR}/cmake")
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()

# Let windows builds auto-export dll symbols
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

#------------------------------------------------------------------------------#
# Silence MSVC warnings
#------------------------------------------------------------------------------#

if(MSVC)
  add_definitions(
    -D_CRT_SECURE_NO_DEPRECATE
    -D_CRT_SECURE_NO_WARNINGS
    -D_SCL_SECURE_NO_DEPRECATE
    -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
  )
endif()

#------------------------------------------------------------------------------#
# Top level options
#------------------------------------------------------------------------------#
include(ADIOSFunctions)

# Default to a debug build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Force C++11 and C99
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT MSVC)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED True)
endif()

# Use meta-compile features if available, otherwise use specific language
# features
if(NOT (CMAKE_VERSION VERSION_LESS 3.9))
  set(ADIOS2_CXX11_FEATURES cxx_std_11)
  set(ADIOS2_C99_FEATURES c_std_99)
else()
  set(ADIOS2_CXX11_FEATURES cxx_auto_type cxx_nullptr)
  set(ADIOS2_C99_FEATURES c_restrict)
endif()

include(CMakeDependentOption)

# Setup shared library defaults.  If explicitly specified somehow, then default 
# to that.  Otherwise base the default on whether or not shared libs are even
# supported.
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
cmake_dependent_option(BUILD_SHARED_LIBS
  "Build shared libraries (so/dylib/dll)." ${SHARED_LIBS_SUPPORTED}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(BUILD_SHARED_LIBS)

# Setup PIC defaults.  If explicitly specified somehow, then default 
# to that.  Otherwise base the default on whether or not shared libs are even
# supported.
if(DEFINED ADIOS2_ENABLE_PIC)
  set(ADIOS2_ENABLE_PIC_DEFAULT ${ADIOS2_ENABLE_PIC})
elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(ADIOS2_ENABLE_PIC_DEFAULT ${CMAKE_POSITION_INDEPENDENT_CODE})
else()
  set(ADIOS2_ENABLE_PIC_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
cmake_dependent_option(ADIOS2_ENABLE_PIC
  "Build with Position Independent Code" ${ADIOS2_ENABLE_PIC_DEFAULT}
  "SHARED_LIBS_SUPPORTED" OFF
)
set(CMAKE_POSITION_INDEPENDENT_CODE ${ADIOS2_ENABLE_PIC})
mark_as_advanced(CMAKE_POSITION_INDEPENDENT_CODE)

<<<<<<< HEAD
adios_option(BZip2     "Enable support for BZip2 transforms" AUTO)
adios_option(ZFP       "Enable support for ZFP transforms" AUTO)
adios_option(SZ        "Enable support for SZ transforms" AUTO)
adios_option(MPI       "Enable support for MPI" AUTO)
adios_option(SST       "Enable support for SST" AUTO)
adios_option(ZeroMQ    "Enable support for ZeroMQ" AUTO)
adios_option(HDF5      "Enable support for the HDF5 engine" AUTO)
adios_option(ADIOS1    "Enable support for the ADIOS 1.x engine" OFF)
adios_option(Python    "Enable support for Python bindings" AUTO)
adios_option(Fortran   "Enable support for Fortran bindings" AUTO)
adios_option(SysVShMem "Enable support for SysV Shared Memory IPC on *NIX" AUTO)
include(${PROJECT_SOURCE_DIR}/cmake/DetectOptions.cmake)

if(ADIOS2_HAVE_MPI)
  # Workaround for MPI forcing the link of C++ bindings
  add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)
=======
# Default to a RelWithDebInfo build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE RelWithDebugInfo)
endif()


set (SRC_LIST dill_util.c dill_pkg.c dill_cplus.c dill_virtual.c virtual.c dill.c vm.c)


CHECK_TYPE_SIZE("void*" CMAKE_SIZEOF_VOID_P)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
set (NATIVE_CG true)
if (("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "i.86") OR
	("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64") OR
	("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "AMD64"))
    if (CMAKE_SIZEOF_VOID_P MATCHES "8")
      	set (NATIVE_ARCH x86_64)
	set (HOST_X86_64 1)
	set (TEST_PERL_FLAGS "-max_arg=4")
	set (ARCH_FILE x86_64)
    else (CMAKE_SIZEOF_VOID_P MATCHES "8")
	set (NATIVE_ARCH x86)
	set (HOST_X86 1)
	set (TEST_PERL_FLAGS -no_float -max_arg=2)
	set (ARCH_FILE x86)
    endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.*")
    if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.5.*")
	set (NATIVE_ARCH arm5)
	set (HOST_ARM5 1)
	set (TEST_PERL_FLAGS -max_arg=4)
	set (ARCH_FILE arm5)
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.6.")
	set (NATIVE_ARCH arm6)
	set (HOST_ARM6 1)
	set (TEST_PERL_FLAGS -max_arg=4)
	set (ARCH_FILE arm6)
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "arm.7.")
	set (NATIVE_ARCH arm6)
	set (HOST_ARM7 1)
	set (TEST_PERL_FLAGS -max_arg=4)
	set (ARCH_FILE arm6)
    else ()
	set (NATIVE_ARCH arm6)
	set (HOST_ARM6 1)
	set (TEST_PERL_FLAGS -max_arg=4)
	set (ARCH_FILE arm6)
    endif()
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "aarch64")
    set (NATIVE_ARCH arm8)
    set (HOST_ARM8 1)
    set (ARCH_FILE arm8)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "ppc64le")
    set (NATIVE_ARCH ppc64le)
    set (HOST_PPC64LE 1)
    set (ARCH_FILE ppc64le)
else()
    message (STATUS "NO CG for Processor \"${CMAKE_SYSTEM_PROCESSOR}\"")
    set (NATIVE_CG false)
    set (NATIVE_ARCH UNSUPPORTED)
endif()

IF ((${CMAKE_SYSTEM_NAME} MATCHES "Darwin") OR (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    set (USE_MMAP_CODE_SEG 1)
ENDIF()

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
set (HARDFP_AVAILABLE 0)
# Determine float ABI of ARM Linux
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(READELF readelf)
    if (READELF MATCHES "NOTFOUND")
        message(WARNING "readelf not found; float ABI detection disabled")
    else (READELF MATCHES "NOTFOUND")
        find_library(MATH_LIBRARY m /usr/lib)
        execute_process(
            COMMAND ${READELF} -A ${MATH_LIBRARY}
            OUTPUT_VARIABLE ELF_ARCH
            ERROR_QUIET)
        if (ELF_ARCH MATCHES "Tag_ABI_VFP_args: VFP registers")
	    set(HARDFP_AVAILABLE 1)
        endif ()
    endif (READELF MATCHES "NOTFOUND")
endif (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
set(ARM_HARD_FLOAT ${HARDFP_AVAILABLE})

option(DILL_IGNORE_NATIVE  "Build to do emulation, regardless of architecture" OFF)
if (DILL_IGNORE_NATIVE)
    set (NATIVE_CG false)
    set (NATIVE_ARCH UNSUPPORTED)
endif()
find_package (LibFFI PATHS ${CMAKE_INSTALL_PREFIX} QUIET)
IF (LIBFFI_FOUND) 
  set (EMULATION_POSSIBLE true)
ELSEIF (DILL_IGNORE_NATIVE)
  set(FFI_PREFIX_DIR ${CMAKE_BINARY_DIR}/libffi)
  set(FFI_SRC_DIR ${FFI_PREFIX_DIR}/src/LibFFI)
  set(FFI_INSTALL_DIR ${FFI_PREFIX_DIR}/install)
  ExternalProject_Add(LibFFI
    PREFIX ${FFI_PREFIX_DIR}
    GIT_REPOSITORY https://github.com/libffi/libffi.git
    CONFIGURE_COMMAND cd ${FFI_SRC_DIR} && ./autogen.sh COMMAND ${FFI_SRC_DIR}/configure --prefix=${FFI_PREFIX_DIR}
    )
  set (EMULATION_POSSIBLE true)
  ExternalProject_Get_property(LibFFI install_dir)
  set (LIBFFI_INCLUDE_DIR ${install_dir}/include)
  link_directories(${install_dir}/lib)
  set (LIBFFI_LIBRARY -lffi)
ENDIF()

option(DILL_ENABLE_DISASSEMBLY
  "enable binutils-based disassembly (default is OFF)" OFF)

set (ARCHITECTURES sparc ppc64le virtual x86 x86_64 ia64 arm5 arm6 arm8)
foreach(_arch ${ARCHITECTURES})
  add_custom_command(
    OUTPUT "dill_${_arch}.c"
    COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/${_arch}.ops
    DEPENDS ${_arch}.ops
  ) 
  list(APPEND arch_files "dill_${_arch}.c" "${_arch}.c" "${_arch}_rt.c")
endforeach()
list(REMOVE_ITEM arch_files virtual_rt.c)

add_custom_command(
  OUTPUT "dill.h" "dill.c"
  COMMAND perl ${CMAKE_CURRENT_SOURCE_DIR}/base.ops
  DEPENDS base.ops
) 

option(DILL_MULTI_TARGET  "Build all available target architectures" OFF)

if(DILL_MULTI_TARGET)
  message(STATUS "arch files is ${arch_files}")
  list(APPEND SRC_LIST ${arch_files})
elseif(NOT "${NATIVE_ARCH}" STREQUAL "UNSUPPORTED") 
  list(APPEND SRC_LIST dill_${ARCH_FILE}.c ${ARCH_FILE}.c ${ARCH_FILE}_rt.c)
>>>>>>> dill 2018-08-13 (84f5763a)
endif()

set(ADIOS2_CONFIG_OPTS
  BZip2 ZFP SZ MPI DataMan SST ZeroMQ HDF5 ADIOS1 Python Fortran SysVShMem
)
GenerateADIOSHeaderConfig(${ADIOS2_CONFIG_OPTS})
configure_file(
  ${PROJECT_SOURCE_DIR}/CTestCustom.cmake.in
  ${PROJECT_BINARY_DIR}/CTestCustom.cmake
  @ONLY
)

install(FILES ${PROJECT_BINARY_DIR}/source/adios2/ADIOSConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)

#------------------------------------------------------------------------------#
# Third party libraries
#------------------------------------------------------------------------------#
include(CTest)
mark_as_advanced(BUILD_TESTING)
add_subdirectory(thirdparty)

#------------------------------------------------------------------------------#
# Main library source
#------------------------------------------------------------------------------#
add_subdirectory(source)

#------------------------------------------------------------------------------#
# Language bindings
#------------------------------------------------------------------------------#
add_subdirectory(bindings)

#------------------------------------------------------------------------------#
# Examples
#------------------------------------------------------------------------------#
option(ADIOS2_BUILD_EXAMPLES "Build examples" ON)
option(ADIOS2_BUILD_EXAMPLES_EXPERIMENTAL "Build experimental examples" OFF)
if(ADIOS2_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

#------------------------------------------------------------------------------#
# Testing
#------------------------------------------------------------------------------#
if(DEFINED BUILD_TESTING)
  set(ADIOS2_BUILD_TESTING_DEFAULT ${BUILD_TESTING})
else()
  set(ADIOS2_BUILD_TESTING_DEFAULT ON)
endif()
unset(BUILD_TESTING)
option(ADIOS2_BUILD_TESTING "Build the ADIOS2 testing tree" 
       ${ADIOS2_BUILD_TESTING_DEFAULT})
       
include(CTest)
set(BUILD_TESTING ${ADIOS2_BUILD_TESTING})
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(testing)
endif()

#------------------------------------------------------------------------------#
# Generating package configs
#------------------------------------------------------------------------------#
GenerateADIOSPackageConfig()

#------------------------------------------------------------------------------#
# Configuration summary
#------------------------------------------------------------------------------#
message("")
message("ADIOS2 build configuration:")
message("  ADIOS Version: ${ADIOS2_VERSION}")
message("  C++ Compiler : ${CMAKE_CXX_COMPILER_ID} " 
                         "${CMAKE_CXX_COMPILER_VERSION} " 
                         "${CMAKE_CXX_COMPILER_WRAPPER}")
message("    ${CMAKE_CXX_COMPILER}")
message("")
if(ADIOS2_HAVE_Fortran)
  message("  Fortran Compiler : ${CMAKE_Fortran_COMPILER_ID} "
                               "${CMAKE_Fortran_COMPILER_VERSION} "
                               "${CMAKE_Fortran_COMPILER_WRAPPER}")
  message("    ${CMAKE_Fortran_COMPILER}")
  message("")
endif()
message("  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message("        bin: ${CMAKE_INSTALL_BINDIR}")
message("        lib: ${CMAKE_INSTALL_LIBDIR}")
message("    include: ${CMAKE_INSTALL_INCLUDEDIR}")
message("      cmake: ${CMAKE_INSTALL_CMAKEDIR}")
if(ADIOS2_HAVE_Python)
  message("     python: ${CMAKE_INSTALL_PYTHONDIR}")
endif()
message("")
message("  Features:")
if(BUILD_SHARED_LIBS)
  set(msg_lib_type "shared")
elseif(CMAKE_POSITION_INDEPENDENT_CODE)
  set(msg_lib_type "static (with PIC)")
else()
  set(msg_lib_type "static (without PIC)")
endif()
message("    Library Type: ${msg_lib_type}")
message("    Build Type:   ${CMAKE_BUILD_TYPE}")
message("    Testing: ${BUILD_TESTING}")
message("    Build Options:")

foreach(opt IN LISTS ADIOS2_CONFIG_OPTS)
  message_pad("      ${opt}" 15 label)
  if(${ADIOS2_HAVE_${opt}})
    message("${label}: ON")
  else()
    message("${label}: OFF")
  endif()
endforeach()
message("")
