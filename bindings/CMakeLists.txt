

if(ADIOS2_HAVE_Python)
  add_subdirectory(python)
endif()

if(ADIOS2_HAVE_C)
  add_subdirectory(c)  
endif()

if(ADIOS2_HAVE_Fortran)
  add_subdirectory(fortran)
endif()
