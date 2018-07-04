cmake_minimum_required(VERSION 2.8.3)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
  project(DILL VERSION 2.3.2 LANGUAGES C CXX)
else()
  project(DILL C CXX)
  set(DILL_MAJOR_VERSION 2)
  set(DILL_MINOR_VERSION 3)
  set(DILL_PATCH_VERSION 3)
  set(DILL_VERSION
    ${DILL_MAJOR_VERSION}.${DILL_MINOR_VERSION}.${DILL_PATCH_VERSION})
endif()

# Some boilerplate to setup nice output directories
set(CMAKE_INSTALL_BINDIR bin CACHE STRING "Installation runtime subdirectory")
set(CMAKE_INSTALL_LIBDIR lib CACHE STRING "Installation library subdirectory")
set(CMAKE_INSTALL_INCLUDEDIR include
  CACHE STRING "Installation include subdirectory")
set(CMAKE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/dill
  CACHE STRING "Installation CMake subdirectory")
mark_as_advanced(CMAKE_INSTALL_BINDIR)
mark_as_advanced(CMAKE_INSTALL_LIBDIR)
mark_as_advanced(CMAKE_INSTALL_INCLUDEDIR)
mark_as_advanced(CMAKE_INSTALL_CMAKEDIR)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
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

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckTypeSize)
include(Compile)
include(TestBigEndian)


# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

set (CMAKE_MACOSX_RPATH 1)

include(CMakeDependentOption)
include(ExternalProject)

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
if(DEFINED DILL_ENABLE_PIC)
  set(DILL_ENABLE_PIC_DEFAULT ${DILL_ENABLE_PIC})
elseif(DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(DILL_ENABLE_PIC_DEFAULT ${CMAKE_POSITION_INDEPENDENT_CODE})
else()
  set(DILL_ENABLE_PIC_DEFAULT ${SHARED_LIBS_SUPPORTED})
endif()
cmake_dependent_option(DILL_ENABLE_PIC
  "Build with Position Independent Code" ${DILL_ENABLE_PIC_DEFAULT}
  "SHARED_LIBS_SUPPORTED" OFF
)
mark_as_advanced(DILL_ENABLE_PIC)
set(CMAKE_POSITION_INDEPENDENT_CODE ${DILL_ENABLE_PIC})
mark_as_advanced(CMAKE_POSITION_INDEPENDENT_CODE)

# Default to a RelWithDebInfo build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE RelWithDebugInfo)
endif()


set (SRC_LIST dill_util.c dill_pkg.c dill_cplus.c dill_virtual.c virtual.c dill.c vm.c)


CHECK_TYPE_SIZE("void*" CMAKE_SIZEOF_VOID_P)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
set (NATIVE_CG true)
if (("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "i.86") OR
	("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64"))
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
endif()

# The MULTI_TARGET variable is only used to generate the preprocessor symbol
# in config.h.  Use the DIL_MULTI_CONFIG option variable for all associated
# logic in the build.
set(MULTI_TARGET ${DILL_MULTI_TARGET})

if(DILL_MULTI_TARGET OR NATIVE_CG OR EMULATION_POSSIBLE)
    if (EMULATION_POSSIBLE)
	set (EMU_LIB ${LIBFFI_LIBRARY})
	INCLUDE_DIRECTORIES(${LIBFFI_INCLUDE_DIR})
    endif(EMULATION_POSSIBLE)
    if ((NOT DILL_MULTI_TARGET) AND (NOT NATIVE_CG))
        message ( STATUS "Configure has detected no native dynamic code generation support 
	for this architecture (\"$NATIVE_ARCH\").  However, the libffi
	library was found, so library configure for emulation will be built.")
	set (EMULATION_ONLY true)
	set (BUILD_EMULATOR true)
	LIST(APPEND TARGET_DEP_LIBS ${EMU_LIB})
    endif()
else ()
    message ( FATAL_ERROR "Configure has detected no native dynamic code generation support 
    for this architecture (\"$NATIVE_ARCH\"), -DDILL_MULTI_TARGET=ON 
	was not specified, and no emulation is possible (libffi library 
	not found)
		       NO LIBRARY WILL BE BUILT")
endif()

CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)

CHECK_INCLUDE_FILES(stdarg.h STDC_HEADERS)
CHECK_INCLUDE_FILES(malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)

set (NO_DISASSEMBLER TRUE)
if (DILL_ENABLE_DISASSEMBLY)
    if (BINUTILS_DIR) 
      set(BINUTILS_LIB_SEARCH ${BINUTILS_DIR}/lib ${BINUTILS_DIR}/lib64)
      set(BINUTILS_INC_SEARCH ${BINUTILS_DIR}/include)
    endif()
    find_library(LIBOPCODES NAMES opcodes HINTS ${BINUTILS_LIBRARIES} PATHS /usr/lib
      /usr/lib64
      /usr/local/lib
      /usr/local/lib64
      /opt/local/lib
      /opt/local/lib64
      ${BINUTILS_LIB_SEARCH}
      ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH)
    find_library(LIBBFD NAMES bfd HINTS ${BINUTILS_LIBRARIES} PATHS /usr/lib
      /usr/lib64
      /usr/local/lib
      /usr/local/lib64
      /opt/local/lib
      /opt/local/lib64
      ${BINUTILS_LIB_SEARCH}
      ENV LIBRARY_PATH
      ENV LD_LIBRARY_PATH)
    find_path (BINUTILS_INCLUDE_DIR
      NAMES
      dis-asm.h
      PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /opt/include
      ${BINUTILS_INC_SEARCH}
      ENV 
      ENV CPATH)
    if (("${LIBOPCODES}" STREQUAL "LIBOPCODES-NOTFOUND") OR 
	("${BINUTILS_INCLUDE_DIR}" STREQUAL "BINUTILS_INCLUDE_DIR-NOTFOUND"))
	message(FATAL_ERROR "libopcodes not found, You need to install binutils for disassembler support.")
    else()
	INCLUDE_DIRECTORIES(${BINUTILS_INCLUDE_DIR})
	LINK_DIRECTORIES(${BINUTILS_LIB_DIR})
	LIST(APPEND TARGET_DEP_LIBS ${BINUTILS_LIBRARIES})
	set(DIS_LIBS ${TARGET_DEP_LIBS})
	set (NO_DISASSEMBLER FALSE)
    endif()
    set (BINUTILS_LIBRARIES ${LIBOPCODES} ${LIBBFD})
    if(NOT DEFINED INIT_DISASSEMBLE_INFO_THREE_ARG)
	#if not in Cache
    	FILE (WRITE check_init_disassemble_args.c "#include \"dis-asm.h\"
extern int main() {struct disassemble_info info;INIT_DISASSEMBLE_INFO(info, stdout, fprintf);}" )
    	TRY_COMPILE (INIT_DISASSEMBLE_INFO_THREE_ARGUMENTS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/check_init_disassemble_args.c CMAKE_FLAGS "-DCMAKE_C_LINK_EXECUTABLE=echo" COMPILE_DEFINITIONS -I${BINUTILS_INCLUDE_DIR} OUTPUT_VARIABLE LOG2)
    	FILE (REMOVE check_init_disassemble_args.c)
	message (STATUS "Checking to see that the INIT_DISASSEMBLE_INFO macro requires three arguments :${INIT_DISASSEMBLE_INFO_THREE_ARGUMENTS}")
	set (INIT_DISASSEMBLE_INFO_THREE_ARG ${INIT_DISASSEMBLE_INFO_THREE_ARGUMENTS} CACHE BOOL "Does the INIT_DISASSEMBLE_INFO macro require three arguments")
    endif()
    SET (CMAKE_REQUIRED_LIBRARIES ${LIBBFD})
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_sparc "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_SPARC)
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_big_powerpc "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_BIG_POWERPC)
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_little_powerpc "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_LITTLE_POWERPC)
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_ia64 "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_IA64)
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_i386 "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_I386)
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_arm "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_ARM)
    CHECK_LIBRARY_EXISTS(${LIBOPCODES} print_insn_little_arm "${BINUTILS_LIB_DIR}" HAVE_PRINT_INSN_LITTLE_ARM)

endif()

set(DILL_LIBRARY_PREFIX "" CACHE STRING
  "Prefix to prepend to the output library name")
mark_as_advanced(DILL_LIBRARY_PREFIX)

add_library(dill ${SRC_LIST})
if (FFI_PREFIX_DIR)
  add_dependencies (dill LibFFI)
endif()
set_target_properties(dill PROPERTIES
  OUTPUT_NAME ${DILL_LIBRARY_PREFIX}dill
  VERSION ${DILL_VERSION}
  SOVERSION ${DILL_VERSION_MAJOR})
add_library(dill::dill ALIAS dill)

target_include_directories(dill PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(dill PRIVATE ${TARGET_DEP_LIBS})

# Setup pkgconfig
set(_pkg_config_private_libs)
foreach(L ${TARGET_DEP_LIBS})
  if(L MATCHES "(.*)/?lib(.*).")
    if(CMAKE_MATCH_1)
      list(APPEND _pkg_config_private_libs "-L${CMAKE_MATCH_1}")
    endif()
    list(APPEND _pkg_config_private_libs "-l${CMAKE_MATCH_2}")
  elseif(L MATCHES "^-")
    list(APPEND _pkg_config_private_libs "${L}")
  else()
    list(APPEND _pkg_config_private_libs "-l${L}")
  endif()
endforeach()
string(REPLACE ";" " " _pkg_config_private_libs "${_pkg_config_private_libs}")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/dill.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/dill.pc
  @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dill.pc
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/dill-config.in
  ${CMAKE_CURRENT_BINARY_DIR}/dill-config
  @ONLY
)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dill-config
  DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dill.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS dill
  # IMPORTANT: Add the dill library to the "export-set"
  EXPORT dill-targets
  RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"          COMPONENT bin
  LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"          COMPONENT shlib
  ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"          COMPONENT lib
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/dill" COMPONENT dev)

if (${CMAKE_C_COMPILER_ID} MATCHES "Intel") 
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -shared-intel")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Add all targets to the build-tree export set
export(TARGETS dill NAMESPACE dill::
  FILE "${PROJECT_BINARY_DIR}/dill-targets.cmake")
 
configure_file(dill-config.cmake.in
  "${PROJECT_BINARY_DIR}/dill-config.cmake" @ONLY)

configure_file(dill-config-version.cmake.in
  "${PROJECT_BINARY_DIR}/dill-config-version.cmake" @ONLY)
 
# Install the dill-config.cmake and dill-config-version.cmake
install(FILES
  "${PROJECT_BINARY_DIR}/dill-config.cmake"
  "${PROJECT_BINARY_DIR}/dill-config-version.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" COMPONENT dev)
 
# Install the export set for use with the install-tree
install(EXPORT dill-targets NAMESPACE dill::
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}" COMPONENT dev)

# display status message for important variables
include(CTest)
mark_as_advanced(BUILD_TESTING)

if(BUILD_TESTING)
  ENABLE_TESTING()
  SET(BUILDNAME "${BUILDNAME}" CACHE STRING "Name of build on the dashboard")
  MARK_AS_ADVANCED(BUILDNAME)
  ADD_SUBDIRECTORY( tests )
  ADD_SUBDIRECTORY( vtests )
endif()

option(DILL_QUIET "Suppress summary output" OFF)
if(NOT DILL_QUIET)
  message(STATUS )
  message(STATUS "-----------------------------------------------------------------------------" )
  message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
  message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}" )
  message(STATUS "DILL_ENABLE_DISASSEMBLY = ${DILL_ENABLE_DISASSEMBLY}" )
  message(STATUS "DILL_MULTI_TARGET = ${DILL_MULTI_TARGET}" )
  message(STATUS "BUILD_TESTING = ${BUILD_TESTING}" )
  message(STATUS "Change a value with: cmake -D<Variable>=<Value>" )
  message(STATUS "-----------------------------------------------------------------------------" )
endif()
