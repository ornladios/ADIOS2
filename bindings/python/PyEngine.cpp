#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>

#include "PyEngineBase.h"
#include <adios2.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<Engine>(pybind11::module &m)
{
    // Wrapping for internal Engine classes
    pybind11::class_<Engine, std::shared_ptr<Engine>> engine(m, "EngineBase");
    engine.def("Write",
               [](Engine &e, VariableBase &variable, pybind11::array values) {
                   e.Write(variable, values.data());
               });
    engine.def("Write",
               [](Engine &e, const std::string name, pybind11::array values) {
                   e.Write(name, values.data());
               });
    engine.def("InquireVariable",
               [](Engine &e, const std::string name) -> VariableBase * {
                   VariableBase *var;
#define inquire(T)                                                             \
    if (var = e.InquireVariable<T>(name, false))                               \
    {                                                                          \
        return var;                                                            \
    }
                   ADIOS2_FOREACH_TYPE_1ARG(inquire)
#undef inquire
                   return nullptr;
               });
    engine.def("Close", &Engine::Close, pybind11::arg("transportIndex") = -1);

    //
    // Wrappings to extend Engine in Python
    //

    // This is the trampoline class used by pybind11 to implement the
    // inheritance model and virtual springboard
    class PyEngine : public PyEngineBase
    {
    public:
        using PyEngineBase::PyEngineBase;
        void Init() override { PYBIND11_OVERLOAD(void, PyEngineBase, Init, ); }
        void DoWrite(VariableBase *var, pybind11::array values) override
        {
            PYBIND11_OVERLOAD_PURE(void, PyEngineBase, DoWrite, var, values);
        }
        void Close(const int transportIndex)
        {
            PYBIND11_OVERLOAD_PURE(void, PyEngineBase, Close, transportIndex);
        }
    };

    pybind11::class_<PyEngineBase, PyEngine, std::shared_ptr<PyEngineBase>>(
        m, "Engine", engine)
        .def(pybind11::init<const std::string, IO &, const std::string,
                            const OpenMode>())
        .def("Init", &PyEngineBase::Init)
        .def("DoWrite",
             (void (PyEngineBase::*)(VariableBase *, pybind11::array)) &
                 PyEngineBase::DoWrite)
        .def("Close", &PyEngineBase::Close);
}
} // end namespace adios2
