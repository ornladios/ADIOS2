if(NOT BUILD_SHARED_LIBS)
  message(ERROR "Python bindings are only supported for shared libraries")
endif()

pybind11_add_module(adios2py MODULE
  ADIOSPy.h
  ADIOSPy.cpp
  adiosPyFunctions.h
  adiosPyFunctions.cpp
  adiosPyTypes.h
  EnginePy.h
  EnginePy.inl
  EnginePy.cpp
  gluePyBind11.cpp
  IOPy.h
  IOPy.cpp
  VariablePy.h
  VariablePy.cpp
)
target_link_libraries(adios2py PRIVATE adios2)
if(ADIOS2_HAVE_MPI)
  find_package(PythonModule REQUIRED COMPONENTS mpi4py mpi4py/mpi4py.h)
  target_link_libraries(adios2py PRIVATE PythonModule::mpi4py)
endif()

string(REGEX REPLACE "^${PYTHON_PREFIX}/" ""
  CMAKE_INSTALL_PYTHONDIR "${PYTHON_SITE_PACKAGES}"
)
set_target_properties(adios2py PROPERTIES
  OUTPUT_NAME adios2
  LIBRARY_OUTPUT_DIRECTORY ${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
  RUNTIME_OUTPUT_DIRECTORY ${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
)
install(TARGETS adios2py 
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}
)
