if(ADIOS2_HAVE_SZ)
  fortran_add_test_helper(BPWriteReadSZ2D TRUE)
  fortran_add_test_helper(BPWriteReadSZ3D TRUE)
endif()

if(ADIOS2_HAVE_ZFP)
  fortran_add_test_helper(BPWriteReadZfp2D TRUE)
endif()
