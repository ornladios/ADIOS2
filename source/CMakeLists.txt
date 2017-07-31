#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(adios2)

install(
  FILES adios2.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
