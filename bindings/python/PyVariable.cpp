#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <adios2.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<VariableBase>(pybind11::module &m)
{
    class PyVariableBase : public VariableBase
    {
    public:
        using VariableBase::VariableBase;
        pybind11::dtype m_DType;
    };

    pybind11::class_<VariableBase, PyVariableBase> varBase(m, "Variable");
    varBase.def_readonly("Name", &VariableBase::m_Name);
    varBase.def_readonly("ElementSize", &VariableBase::m_ElementSize);
    varBase.def_readonly("Shape", &VariableBase::m_Shape);
    varBase.def_readonly("Start", &VariableBase::m_Start);
    varBase.def_readonly("Count", &VariableBase::m_Count);
    varBase.def_readonly("IsScalar", &VariableBase::m_SingleValue);
    varBase.def_readonly("HasConstantDims", &VariableBase::m_ConstantDims);
    varBase.def_property_readonly("PayLoadSize", &VariableBase::PayLoadSize);
    varBase.def_property_readonly("TotalSize", &VariableBase::TotalSize);
    varBase.def_property_readonly(
        "Type", [](PyVariableBase &self) { return self.m_DType; });

#define pyvar(T, L)                                                            \
    pybind11::class_<Variable<T>>(m, "Variable" ADIOS2_STRINGIFY(L), varBase)  \
        .def_readwrite("Data", &Variable<T>::m_Data);
    ADIOS2_FOREACH_TYPE_2ARGS(pyvar)
#undef pyvar
}

} // end namespace adios2
