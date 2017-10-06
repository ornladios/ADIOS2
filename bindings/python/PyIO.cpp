#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <adios2.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<IO>(pybind11::module &m)
{
    pybind11::class_<IO>(m, "IO")
        .def_property("EngineType", &IO::GetEngine, &IO::SetEngine)
        .def_property("Parameters", (Params & (IO::*)()) & IO::GetParameters,
                      &IO::SetParameters)
        .def("AddTransport", &IO::AddTransport)
        .def_property_readonly("TransportParameters",
                               &IO::GetTransportParameters)
        .def("DefineVariable",
             [](IO &io, std::string &name, Dims &shape, Dims &start,
                Dims &count, bool constantDims, pybind11::object type) {
                 pybind11::dtype dt = pybind11::dtype::from_args(type);
                 if (type.is_none())
                 {
                     // Delay variable creation
                 }
#define define_variable(T)                                                     \
    else if (dt.type() == pybind11::dtype::of<T>().type())                     \
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
             pybind11::arg("type").none(false))
        .def("RemoveVariable", &IO::RemoveVariable,
             pybind11::arg("name").none(false))
        .def("GetVariable", &IO::GetVariableBase,
             pybind11::arg("name").none(false))
        .def("Open", [](IO &io, const std::string &name,
                        const OpenMode mode) {
            return io.Open(name, mode);
        }, pybind11::return_value_policy::reference);
}

} // end namespace adios2
