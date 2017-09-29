#include <adios2.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace adios2
{
template<typename T> void GeneratePythonBindings(pybind11::module &m);

template<>
void GeneratePythonBindings<VariableBase>(pybind11::module &m)
{
    pybind11::class_<VariableBase>(m, "Variable")
        .def_readonly("Name", &VariableBase::m_Name)
        .def_readonly("Type", &VariableBase::m_Type)
        .def_readonly("ElementSize", &VariableBase::m_ElementSize)
        .def_readonly("Shape", &VariableBase::m_Shape)
        .def_readonly("Start", &VariableBase::m_Start)
        .def_readonly("Count", &VariableBase::m_Count)
        .def_readonly("IsScalar", &VariableBase::m_SingleValue)
        .def_readonly("HasConstantDims", &VariableBase::m_ConstantDims)
        .def_property_readonly("PayLoadSize",
                               &VariableBase::PayLoadSize)
        .def_property_readonly("TotalSize", &VariableBase::TotalSize);

#define pyvar(T, L)                                                            \
    pybind11::class_<Variable<T>, VariableBase>(                               \
        m, "Variable" ADIOS2_STRINGIFY(L))                                     \
        .def_readwrite("Data", &Variable<T>::m_Data);
    ADIOS2_FOREACH_TYPE_2ARGS(pyvar)
#undef pyvar

}

} // end namespace adios2
