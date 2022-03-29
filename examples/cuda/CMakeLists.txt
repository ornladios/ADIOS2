#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(CudaBP5WriteRead_cuda cudaBP5WriteRead.cu)
target_link_libraries(CudaBP5WriteRead_cuda PUBLIC adios2::cxx11 CUDA::cudart)
set_target_properties(CudaBP5WriteRead_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

add_executable(CudaBP4WriteRead_cuda cudaBP4WriteRead.cu)
target_link_libraries(CudaBP4WriteRead_cuda PUBLIC adios2::cxx11 CUDA::cudart)
set_target_properties(CudaBP4WriteRead_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
