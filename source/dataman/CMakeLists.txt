#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(dataman
  DataManBase.h DataManBase.cpp
  DataMan.h DataMan.cpp
)
target_include_directories(dataman PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dataman PUBLIC ${CMAKE_DL_LIBS})

install(
  TARGETS dataman EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
