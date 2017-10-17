#include <adios2.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<Selection>(pybind11::module &m)
{
    pybind11::class_<Selection>(m, "Selection")
        .def_readonly("Type", &Selection::m_Type);

    pybind11::class_<SelectionBoundingBox>("SelectionBoundingBox")
        .def(pybind11::init<const Dims, const Dims, vonst bool>(),
             pybind11::arg("start"), pybind11::arg("count"),
             pybind11::arg("debug") = false)
        .def_readwrite("Start", &SelectionBoundingBox.m_Start)
        .def_readwrite("Count", &SelectionBoundingBox.m_Count);
}

} // end namespace adios2
