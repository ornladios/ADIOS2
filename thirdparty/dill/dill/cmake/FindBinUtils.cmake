#
#  BINUTILS_FOUND - system has the BinUtils library BINUTILS_INCLUDE_DIR - the BinUtils include directory BINUTILS_LIBRARIES - The libraries needed
#  to use BinUtils

IF (NOT DEFINED BINUTILS_FOUND)
    if (NOT (DEFINED CercsArch))
        execute_process(COMMAND cercs_arch OUTPUT_VARIABLE CercsArch ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	MARK_AS_ADVANCED(CercsArch)
    endif()
    IF(EXISTS /users/c/chaos)
        FIND_LIBRARY  (BINUTILS_LIBRARIES opcodes NAMES libopcodes.so opcodes PATHS /users/c/chaos/lib /users/c/chaos/${CercsArch}/binutils/lib /users/c/chaos/${CercsArch}/lib NO_DEFAULT_PATH)
	FIND_PATH (BINUTILS_INCLUDE_DIR NAMES dis-asm.h PATHS /users/c/chaos/include /users/c/chaos/${CercsArch}/binutils/include /users/c/chaos/${CercsArch}/include NO_DEFAULT_PATH)
    ENDIF()
    if (USE_NATIVE_LIBRARIES)
	FIND_LIBRARY (BINUTILS_LIBRARIES opcodes NAMES libopcodes.so opcodes)
	FIND_PATH (BINUTILS_INCLUDE_DIR  NAMES dis-asm.h)
    endif()
    IF (DEFINED BINUTILS_LIBRARIES)
	get_filename_component ( BINUTILS_LIB_DIR ${BINUTILS_LIBRARIES} PATH)
    ENDIF()
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(BinUtils DEFAULT_MSG
     BINUTILS_LIBRARIES
     BINUTILS_INCLUDE_DIR
     BINUTILS_LIB_DIR
   )
ENDIF()
