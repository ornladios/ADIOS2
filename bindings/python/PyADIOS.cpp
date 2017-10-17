#include <pybind11/pybind11.h>

#include <adios2.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi4py/mpi4py.h>
#endif

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<ADIOS>(pybind11::module &m)
{
    pybind11::class_<ADIOS>(m, "ADIOS")
        .def(pybind11::init([](const bool debug) {
                 auto a = new ADIOS(debug);
                 a->m_HostLanguage = "Python";
                 return a;
             }),
             pybind11::arg("debug") = true)
        .def(pybind11::init([](const std::string config, const bool debug) {
                 auto a = new ADIOS(config, debug);
                 a->m_HostLanguage = "Python";
                 return a;
             }),
             pybind11::arg("config").none(false), pybind11::arg("debug") = true)
#ifdef ADIOS2_HAVE_MPI
        .def(pybind11::init([](pybind11::object comm, const bool debug) {
                 import_mpi4py__MPI();
                 auto a = new ADIOS(*PyMPIComm_Get(comm.ptr()), debug);
                 a->m_HostLanguage = "Python";
                 return a;
             }),
             pybind11::arg("comm").none(false), pybind11::arg("debug") = true)
        .def(pybind11::init([](pybind11::object comm, const std::string config,
                               const bool debug) {
                 import_mpi4py__MPI();
                 auto a = new ADIOS(config, *PyMPIComm_Get(comm.ptr()), debug);
                 a->m_HostLanguage = "Python";
                 return a;
             }),
             pybind11::arg("comm").none(false),
             pybind11::arg("config").none(false), pybind11::arg("debug") = true)
#endif
        .def_readonly("HostLanguage", &ADIOS::m_HostLanguage)
        .def("DeclareIO", &ADIOS::DeclareIO)
        .def("GetIO", &ADIOS::GetIO);
}

} // end namespace adios2
