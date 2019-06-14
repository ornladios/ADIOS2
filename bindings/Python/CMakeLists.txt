pybind11_add_module(adios2py MODULE
  py11ADIOS.cpp
  py11IO.cpp
  py11Variable.cpp
  py11Attribute.cpp
  py11Engine.cpp
  py11Operator.cpp
  py11File.cpp py11File.tcc
  py11glue.cpp
)
target_link_libraries(adios2py PRIVATE adios2)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(adios2py PRIVATE PythonModule::mpi4py)
endif()

set(CMAKE_INSTALL_PYTHONDIR
  "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages"
)
set(CMAKE_INSTALL_PYTHONDIR "${CMAKE_INSTALL_PYTHONDIR}"
  CACHE INTERNAL "" FORCE
)
set_target_properties(adios2py PROPERTIES
  OUTPUT_NAME adios2
  LIBRARY_OUTPUT_DIRECTORY ${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
  RUNTIME_OUTPUT_DIRECTORY ${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}
)
install(TARGETS adios2py
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}
)
