#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


add_subdirectory(cwriter)
add_subdirectory(changingshape)

if(ADIOS2_HAVE_MPI)
  add_subdirectory(iotest)
endif(ADIOS2_HAVE_MPI)
