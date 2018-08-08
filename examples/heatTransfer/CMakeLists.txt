#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(write)
add_subdirectory(read)
add_subdirectory(read_fileonly)

include(${CMAKE_CURRENT_SOURCE_DIR}/TestBPFileMxM.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestBPFileMxN.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestBPFileMx1.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTBPMxM.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTBPMxN.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTBPMx1.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTFFSMxM.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTFFSMxN.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTFFSMx1.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestInsituMPIMxM.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestInsituMPIMxN.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/TestInsituMPIMx1.cmake)

