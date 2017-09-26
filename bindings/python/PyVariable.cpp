#include <adios2.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace adios2
{
template<typename T> void GeneratePythonBindings(pybind11::module &m);

template<>
void GeneratePythonBindings<VariableBase>(pybind11::module &m)
{
    pybind11::class_<adios2::VariableBase>(m, "Variable")
        .def_readonly("Name", &adios2::VariableBase::m_Name)
        .def_readonly("Type", &adios2::VariableBase::m_Type)
        .def_readonly("ElementSize", &adios2::VariableBase::m_ElementSize)
        .def_readonly("Shape", &adios2::VariableBase::m_Shape)
        .def_readonly("Start", &adios2::VariableBase::m_Start)
        .def_readonly("Count", &adios2::VariableBase::m_Count)
        .def_readonly("IsScalar", &adios2::VariableBase::m_SingleValue)
        .def_readonly("HasConstantDims", &adios2::VariableBase::m_ConstantDims)
        .def_property_readonly("PayLoadSize",
                               &adios2::VariableBase::PayLoadSize)
        .def_property_readonly("TotalSize", &adios2::VariableBase::TotalSize);
#define pyvar(T, L)                                                            \
    pybind11::class_<adios2::Variable<T>, adios2::VariableBase>(               \
        m, "Variable" ADIOS2_STRINGIFY(L))                                     \
        .def_readwrite("Data", &adios2::Variable<T>::m_Data);
    ADIOS2_FOREACH_TYPE_2ARGS(pyvar)
#undef pyvar

}

} // end namespace adios2
