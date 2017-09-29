#include <adios2.h>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<IO>(pybind11::module &m)
{
    pybind11::class_<IO>(m, "IO")
        .def_property("EngineType", &IO::SetEngine, &IO::GetEngine)
        .def_property("Parameters", &IO::SetParameters,
                      (Params & (IO::*)()) & IO::GetParameters)
        .def("SetIOMode", &IO::SetIOMode)
        .def("AddTransport", &IO::AddTransport)
        .def_property_readonly("TransportParameters",
                               &IO::GetTransportParameters)
        .def("DefineVariable",
             [](IO &io, std::string &name, Dims &shape, Dims &start,
                Dims &count, bool constantDims, pybind11::dtype type) {
                 if (type.is_none())
                 {
                     // Delay variable creation
                 }
#define define_variable(T)                                                     \
    else if (type.kind() == pybind11::dtype::of<T>().kind())                   \
    {                                                                          \
        return pybind11::cast(                                                 \
            io.DefineVariable<T>(name, shape, start, count, constantDims));    \
    }
                 ADIOS2_FOREACH_TYPE_1ARG(define_variable)
#undef define_variable
                 else
                 {
                     throw std::invalid_argument(
                         "Error: IO::DefineVariable called "
                         "with unsupported datatype");
                 }
             },
             pybind11::arg("name").none(false),
             pybind11::arg("shape").none(false) = Dims{},
             pybind11::arg("start").none(false) = Dims{},
             pybind11::arg("count").none(false) = Dims{},
             pybind11::arg("constantDims") = false,
             pybind11::arg("type").none(false));
}

} // end namespace adios2
