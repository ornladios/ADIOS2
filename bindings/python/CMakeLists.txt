find_package(PythonInterp REQUIRED)
find_package(PythonModule REQUIRED COMPONENTS mpi4py mpi4py/mpi4py.h)

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
target_link_libraries(adios2py PUBLIC adios2 PythonModule::mpi4py)

set(python_package_dir ${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}/site-packages)
set_target_properties(adios2py PROPERTIES
  PREFIX ""
  OUTPUT_NAME adios2
  LIBRARY_OUTPUT_DIRECTORY ${ADIOS_BINARY_DIR}/${python_package_dir}
  RUNTIME_OUTPUT_DIRECTORY ${ADIOS_BINARY_DIR}/${python_package_dir}
)
install(TARGETS adios2py
  DESTINATION ${python_package_dir}
)
