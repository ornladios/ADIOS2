#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

#BPLS2
add_executable(bpls2 ./bpls2/main.cpp ./bpls2/BPLS2.cpp Utils.cpp)

target_link_libraries(bpls2 adios2)

install(TARGETS bpls2 EXPORT adios2
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
