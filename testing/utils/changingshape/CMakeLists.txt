#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestUtilsChangingShape TestUtilsChangingShape.cpp)
target_link_libraries(TestUtilsChangingShape adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestUtilsChangingShape MPI::MPI_C)
  set(cmd_executor ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1)
else()
  set(cmd_executor)
endif()

add_test(NAME Utils.ChangingShape
  COMMAND ${cmd_executor} $<TARGET_FILE:TestUtilsChangingShape>
)
# This test produces TestUtilsChangingShape.bp


# bpls -la to screen for testing
add_test(NAME Utils.ChangingShape.Bpls.la.Screen
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-la
    -DINPUT_FILE=TestUtilsChangingShape.bp
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.ChangingShape.Bpls.la.Screen
  PROPERTY DEPENDS Utils.ChangingShape
)


# bpls -la
add_test(NAME Utils.ChangingShape.Bpls.la.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-la
    -DINPUT_FILE=TestUtilsChangingShape.bp
    -DOUTPUT_FILE=TestUtilsChangingShape.bplsla.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.ChangingShape.Bpls.la.Dump
  PROPERTY DEPENDS Utils.ChangingShape
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.ChangingShape.Bpls.la.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsChangingShape.bplsla.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsChangingShape.bplsla.result.txt
)
set_property(TEST Utils.ChangingShape.Bpls.la.Validate
  PROPERTY DEPENDS Utils.ChangingShape.Bpls.la.Dump
)

endif(ADIOS2_HAVE_MPI)


# bpls -ld  AlternatingStepsAndChangingShapeVar
add_test(NAME Utils.ChangingShape.Bpls.ldAlternatingStepsAndChangingShapeVar.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-ld
    -DARG2=AlternatingStepsAndChangingShapeVar
    -DINPUT_FILE=TestUtilsChangingShape.bp
    -DOUTPUT_FILE=TestUtilsChangingShape.bplsldAlternatingStepsAndChangingShapeVar.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.ChangingShape.Bpls.ldAlternatingStepsAndChangingShapeVar.Dump
  PROPERTY DEPENDS Utils.ChangingShape
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.ChangingShape.Bpls.ldAlternatingStepsAndChangingShapeVar.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsChangingShape.bplsldAlternatingStepsAndChangingShapeVar.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsChangingShape.bplsldAlternatingStepsAndChangingShapeVar.result.txt
)
set_property(TEST Utils.ChangingShape.Bpls.ldAlternatingStepsAndChangingShapeVar.Validate
  PROPERTY DEPENDS Utils.ChangingShape.Bpls.ldAlternatingStepsAndChangingShapeVar.Dump
)

endif(ADIOS2_HAVE_MPI)

# bpls -ld AlternatingStepsVar -s "1,0,0" -c "3,-1,-1" -n 8
add_test(NAME Utils.ChangingShape.Bpls.ldAlternatingStepsVarSelection.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-ld
    -DARG2=AlternatingStepsVar
    -DARG3=-s
    -DARG4=1,0,0
    -DARG5=-c
    -DARG6=3,-1,-1
    -DARG7=-n
    -DARG8=8
    -DINPUT_FILE=TestUtilsChangingShape.bp
    -DOUTPUT_FILE=TestUtilsChangingShape.bplsldAlternatingStepsVarSelection.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.ChangingShape.Bpls.ldAlternatingStepsVarSelection.Dump
  PROPERTY DEPENDS Utils.ChangingShape
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.ChangingShape.Bpls.ldAlternatingStepsVarSelection.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsChangingShape.bplsldAlternatingStepsVarSelection.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsChangingShape.bplsldAlternatingStepsVarSelection.result.txt
)
set_property(TEST Utils.ChangingShape.Bpls.ldAlternatingStepsVarSelection.Validate
  PROPERTY DEPENDS Utils.ChangingShape.Bpls.ldAlternatingStepsVarSelection.Dump
)

endif(ADIOS2_HAVE_MPI)

# bpls -ld ChangingShapeVar -s "5,0,0" -c "1,-1,-1" -n 12
add_test(NAME Utils.ChangingShape.Bpls.ldChangingShapeVarOneStep.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-ld
    -DARG2=ChangingShapeVar
    -DARG3=-s
    -DARG4=5,0,0
    -DARG5=-c
    -DARG6=1,-1,-1
    -DARG7=-n
    -DARG8=12
    -DINPUT_FILE=TestUtilsChangingShape.bp
    -DOUTPUT_FILE=TestUtilsChangingShape.bplsldChangingShapeVarOneStep.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.ChangingShape.Bpls.ldChangingShapeVarOneStep.Dump
  PROPERTY DEPENDS Utils.ChangingShape
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.ChangingShape.Bpls.ldChangingShapeVarOneStep.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsChangingShape.bplsldChangingShapeVarOneStep.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsChangingShape.bplsldChangingShapeVarOneStep.result.txt
)
set_property(TEST Utils.ChangingShape.Bpls.ldChangingShapeVarOneStep.Validate
  PROPERTY DEPENDS Utils.ChangingShape.Bpls.ldChangingShapeVarOneStep.Dump
)


endif(ADIOS2_HAVE_MPI)

# bpls -ld FixedShapeVar -s "5,0,0" -c "10,-1,-1"
add_test(NAME Utils.ChangingShape.Bpls.ldFixedShapeVarTooManySteps.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-ld
    -DARG2=FixedShapeVar
    -DARG3=-s
    -DARG4=5,0,0
    -DARG5=-c
    -DARG6=10,-1,-1
    -DINPUT_FILE=TestUtilsChangingShape.bp
    -DOUTPUT_FILE=TestUtilsChangingShape.bplsldFixedShapeVarTooManySteps.result.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST Utils.ChangingShape.Bpls.ldFixedShapeVarTooManySteps.Dump
  PROPERTY DEPENDS Utils.ChangingShape
)

if(ADIOS2_HAVE_MPI)

add_test(NAME Utils.ChangingShape.Bpls.ldFixedShapeVarTooManySteps.Validate
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsChangingShape.bplsldFixedShapeVarTooManySteps.expected.txt
    ${CMAKE_CURRENT_BINARY_DIR}/TestUtilsChangingShape.bplsldFixedShapeVarTooManySteps.result.txt
)
set_property(TEST Utils.ChangingShape.Bpls.ldFixedShapeVarTooManySteps.Validate
  PROPERTY DEPENDS Utils.ChangingShape.Bpls.ldFixedShapeVarTooManySteps.Dump
)


endif(ADIOS2_HAVE_MPI)


