#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_executable(Test.Engine.Skeleton.Writer TestSkeletonWriter.cpp)
target_link_libraries(Test.Engine.Skeleton.Writer adios2)

add_executable(Test.Engine.Skeleton.Reader TestSkeletonReader.cpp)
target_link_libraries(Test.Engine.Skeleton.Reader adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(Test.Engine.Skeleton.Writer MPI::MPI_C)
  target_link_libraries(Test.Engine.Skeleton.Reader MPI::MPI_C)
  set(MPIEXEC_COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1)
endif()

add_test(NAME Engine.Skeleton.Writer
  COMMAND ${cmd_executor} $<TARGET_FILE:Test.Engine.Skeleton.Writer> 
)

add_test(NAME Engine.Skeleton.Reader
  COMMAND ${cmd_executor} $<TARGET_FILE:Test.Engine.Skeleton.Reader>
)

if(ADIOS2_HAVE_MPI)
  add_test(NAME Engine.Skeleton.Writer.Validate
    COMMAND ${DIFF_EXECUTABLE} -u
      ${CMAKE_CURRENT_SOURCE_DIR}/TestSkeletonWriterExpectedOutput.txt
      TestSkeletonWriterOutput.txt
  )
  SetupTestPipeline(Engine.Skeleton "Writer;Writer.Validate" TRUE)

  add_test(NAME Engine.Skeleton.Reader.Validate
    COMMAND ${DIFF_EXECUTABLE} -u
      ${CMAKE_CURRENT_SOURCE_DIR}/TestSkeletonReaderExpectedOutput.txt
      TestSkeletonReaderOutput.txt
  )
  SetupTestPipeline(Engine.Skeleton "Writer;Reader;Reader.Validate" FALSE)
else()
  SetupTestPipeline(Engine.Skeleton "Writer;Reader" TRUE)
endif()
