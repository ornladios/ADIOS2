#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

enable_language(CUDA)

add_executable(GPUWriteRead_cuda cudaWriteRead.cu)
target_link_libraries(GPUWriteRead_cuda PUBLIC adios2::cxx11 CUDA::cudart CUDA::cuda_driver)
set_target_properties(GPUWriteRead_cuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
