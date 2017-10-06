#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <adios2.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<AttributeBase>(pybind11::module &m)
{
    pybind11::class_<AttributeBase>(m, "Attribute")
        .def_readonly("Name", &AttributeBase::m_Name)
        .def_readonly("Type", &AttributeBase::m_Type)
        .def_readonly("Elements", &AttributeBase::m_Elements)
        .def_readonly("Name", &AttributeBase::m_IsSingleValue);

#define pyattr(T, L)                                                           \
    pybind11::class_<Attribute<T>, AttributeBase>(                             \
        m, "Attribute" ADIOS2_STRINGIFY(L))                                    \
        .def_readwrite("DataArray", &Attribute<T>::m_DataArray)                \
        .def_readwrite("DataSingleValue", &Attribute<T>::m_DataSingleValue);
    ADIOS2_FOREACH_TYPE_2ARGS(pyattr)
#undef pyvar
}

} // end namespace adios2
