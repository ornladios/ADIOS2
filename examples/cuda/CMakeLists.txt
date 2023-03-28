#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(CudaBPWriteRead_cuda cudaBPWriteRead.cu)
target_link_libraries(CudaBPWriteRead_cuda PUBLIC adios2::cxx11 CUDA::cudart)
set_target_properties(CudaBPWriteRead_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
