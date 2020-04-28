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
if(ADIOS2_HAVE_MPI)
  set(maybe_adios2_cxx11_mpi adios2_cxx11_mpi)
  set(maybe_adios2_core_mpi adios2_core_mpi)
  set(maybe_mpi4py Python::mpi4py)
else()
  set(maybe_adios2_cxx11_mpi)
  set(maybe_adios2_core_mpi)
  set(maybe_mpi4py)
endif()
target_link_libraries(adios2py PRIVATE
  ${maybe_adios2_cxx11_mpi} adios2_cxx11
  ${maybe_adios2_core_mpi} adios2_core
  adios2::thirdparty::pybind11
  ${maybe_mpi4py} Python::NumPy
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
  ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/adios2/__init__.py
  COPYONLY
)

set_target_properties(adios2py PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  OUTPUT_NAME adios2
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/adios2
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/adios2
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/adios2
  PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/adios2
  COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PYTHON_OUTPUT_DIRECTORY}/adios2
)
install(TARGETS adios2py
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/adios2
)
install(FILES __init__.py
  DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/adios2
)
