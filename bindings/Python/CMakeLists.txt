Python_add_library(adios2py MODULE
  py11ADIOS.cpp
  py11IO.cpp
  py11Variable.cpp
  py11Attribute.cpp
  py11Engine.cpp
  py11Operator.cpp
  py11File.cpp py11File.tcc
  py11glue.cpp
)
target_link_libraries(adios2py PRIVATE adios2 pybind11 Python::NumPy)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(adios2py PRIVATE Python::mpi4py)
endif()

set_target_properties(adios2py PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  OUTPUT_NAME adios2
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
  PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
  COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}
)
install(TARGETS adios2py
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}
)
