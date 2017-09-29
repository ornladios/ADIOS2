#include <adios2.h>

#include <pybind11/pybind11.h>

namespace adios2
{
  template<typename T>
  void GeneratePythonBindings(pybind11::module &m);
}

PYBIND11_MODULE(adios2, m)
{
    adios2::GeneratePythonBindings<void>(m);
    adios2::GeneratePythonBindings<adios2::AttributeBase>(m);
    adios2::GeneratePythonBindings<adios2::VariableBase>(m);
    adios2::GeneratePythonBindings<adios2::IO>(m);
    adios2::GeneratePythonBindings<adios2::ADIOS>(m);
}
