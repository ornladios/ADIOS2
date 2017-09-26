#include <adios2.h>

#include <pybind11/pybind11.h>

namespace adios2
{
template<typename T> void GeneratePythonBindings(pybind11::module &m);

template<>
void GeneratePythonBindings<IO>(pybind11::module &m)
{
    pybind11::class_<adios2::IO>(m, "IO")
        .def_property("EngineType", &adios2::IO::SetEngine,
                      &adios2::IO::GetEngine)
        .def_property("Parameters", &adios2::IO::SetParameters,
                      &adios2::IO::GetParameters)
        .def("SetIOMode", &adios2::IO::SetIOMode)
        .def("AddTransport", &adios2::IO::AddTransport)
        .def_property_readonly("TransportParameters",
                               &adios2::IO::GetTransportParameters)
        .def("DefineVariable",
             [](adios2::IO &io, std::string &name, adios2::Dims &shape,
                adios2::Dims &start, adios2::Dims &count, bool constantDims,
                int t) {
                 if (t == 1)
                     return pybind11::cast(io.DefineVariable<int>(
                         name, shape, start, count, constantDims));
                 else
                     return pybind11::cast(io.DefineVariable<float>(
                         name, shape, start, count, constantDims));
             });
}

} // end namespace adios2
