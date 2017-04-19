#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
  
if(ADIOS_USE_DataMan)
  add_subdirectory(dataman)
endif()

add_subdirectory(adios2)

install(
  FILES adios2.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
