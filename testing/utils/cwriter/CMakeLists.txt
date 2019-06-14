#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestUtilsCWriter TestUtilsCWriter.c)
target_link_libraries(TestUtilsCWriter adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestUtilsCWriter MPI::MPI_C)
  set(cmd_executor ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1)
else()
  set(cmd_executor)
endif()

add_test(NAME Utils.C.Writer
  COMMAND ${cmd_executor} $<TARGET_FILE:TestUtilsCWriter>
)
# This test produces TestUtilsCWriter.bp

# bpls -h
add_test(NAME Utils.C.Writer.Bpls.h.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-h
    -DINPUT_FILE=TestUtilsCWriter.bp
    -DOUTPUT_FILE=TestUtilsCWriter.bplsh.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.h.Dump
  PROPERTY DEPENDS Utils.C.Writer
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.C.Writer.Bpls.h.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsCWriter.bplsh.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsCWriter.bplsh.result.txt
)
set_property(TEST Utils.C.Writer.Bpls.h.Validate
  PROPERTY DEPENDS Utils.C.Writer.Bpls.h.Dump
)

endif(ADIOS2_HAVE_MPI)


# bpls -la to screen for testing
add_test(NAME Utils.C.Writer.Bpls.la.Screen
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-la
    -DINPUT_FILE=TestUtilsCWriter.bp
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.la.Screen
  PROPERTY DEPENDS Utils.C.Writer
)


# bpls -la
add_test(NAME Utils.C.Writer.Bpls.la.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-la
    -DINPUT_FILE=TestUtilsCWriter.bp
    -DOUTPUT_FILE=TestUtilsCWriter.bplsla.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.la.Dump
  PROPERTY DEPENDS Utils.C.Writer
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.C.Writer.Bpls.la.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsCWriter.bplsla.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsCWriter.bplsla.result.txt
)
set_property(TEST Utils.C.Writer.Bpls.la.Validate
  PROPERTY DEPENDS Utils.C.Writer.Bpls.la.Dump
)

endif(ADIOS2_HAVE_MPI)


# bpls -Al
add_test(NAME Utils.C.Writer.Bpls.Al.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-Al
    -DINPUT_FILE=TestUtilsCWriter.bp
    -DOUTPUT_FILE=TestUtilsCWriter.bplsAl.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.Al.Dump
  PROPERTY DEPENDS Utils.C.Writer
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.C.Writer.Bpls.Al.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsCWriter.bplsAl.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsCWriter.bplsAl.result.txt
)
set_property(TEST Utils.C.Writer.Bpls.Al.Validate
  PROPERTY DEPENDS Utils.C.Writer.Bpls.Al.Dump
)

endif(ADIOS2_HAVE_MPI)

# bpls -ldDav
add_test(NAME Utils.C.Writer.Bpls.ldDav.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1='-ldDav'
    -DINPUT_FILE=TestUtilsCWriter.bp
    -DOUTPUT_FILE=TestUtilsCWriter.bplsldDav.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.ldDav.Dump
  PROPERTY DEPENDS Utils.C.Writer
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.C.Writer.Bpls.ldDav.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsCWriter.bplsldDav.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsCWriter.bplsldDav.result.txt
)
set_property(TEST Utils.C.Writer.Bpls.ldDav.Validate
  PROPERTY DEPENDS Utils.C.Writer.Bpls.ldDav.Dump
)


endif(ADIOS2_HAVE_MPI)


# bpls -ldDavvv
add_test(NAME Utils.C.Writer.Bpls.ldDavvv.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1='-ldDavvv'
    -DINPUT_FILE=TestUtilsCWriter.bp
    -DOUTPUT_FILE=TestUtilsCWriter.bplsldDavvv.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.ldDavvv.Dump
  PROPERTY DEPENDS Utils.C.Writer
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.C.Writer.Bpls.ldDavvv.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsCWriter.bplsldDavvv.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsCWriter.bplsldDavvv.result.txt
)
set_property(TEST Utils.C.Writer.Bpls.ldDavvv.Validate
  PROPERTY DEPENDS Utils.C.Writer.Bpls.ldDavvv.Dump
)


endif(ADIOS2_HAVE_MPI)


# bpls -ld varI16 -n 10
add_test(NAME Utils.C.Writer.Bpls.ldvarI16.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-ld
    -DARG2=varI16
    -DARG3=-n
    -DARG4=10
   -DINPUT_FILE=TestUtilsCWriter.bp
    -DOUTPUT_FILE=TestUtilsCWriter.bplsldvarI16.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.C.Writer.Bpls.ldvarI16.Dump
  PROPERTY DEPENDS Utils.C.Writer
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.C.Writer.Bpls.ldvarI16.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsCWriter.bplsldvarI16.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsCWriter.bplsldvarI16.result.txt
)
set_property(TEST Utils.C.Writer.Bpls.ldvarI16.Validate
  PROPERTY DEPENDS Utils.C.Writer.Bpls.ldvarI16.Dump
)


endif(ADIOS2_HAVE_MPI)


