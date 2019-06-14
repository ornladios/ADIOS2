#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


add_executable(TestSkeletonWriter TestSkeletonWriter.cpp)
target_link_libraries(TestSkeletonWriter adios2)

add_executable(TestSkeletonReader TestSkeletonReader.cpp)
target_link_libraries(TestSkeletonReader adios2)


if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestSkeletonWriter MPI::MPI_C)
  target_link_libraries(TestSkeletonReader MPI::MPI_C)
  set(cmd_executor ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1)
else()
  set(cmd_executor)
endif()

add_test(NAME Skeleton.Writer
  COMMAND ${cmd_executor} $<TARGET_FILE:TestSkeletonWriter> 
)

add_test(NAME Skeleton.Reader
  COMMAND ${cmd_executor} $<TARGET_FILE:TestSkeletonReader> 
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Skeleton.Writer.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSkeletonWriterExpectedOutput.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestSkeletonWriterOutput.txt
)
set_property(TEST Skeleton.Writer.Validate
  PROPERTY DEPENDS Skeleton.Writer
)


add_test(NAME Skeleton.Reader.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestSkeletonReaderExpectedOutput.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestSkeletonReaderOutput.txt
)
set_property(TEST Skeleton.Reader.Validate
  PROPERTY DEPENDS Skeleton.Reader
)


endif(ADIOS2_HAVE_MPI)


