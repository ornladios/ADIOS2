#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>

#include <adios2.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<Engine>(pybind11::module &m)
{
    // Wrapping for internal Engine classes
    pybind11::class_<Engine, std::shared_ptr<Engine>>(m, "EngineBase")
        .def("Write",
             [](Engine &e, VariableBase &variable, pybind11::array values) {
                 e.Write(variable, values.data());
             })
        .def("Write",
             [](Engine &e, const std::string name, pybind11::array values) {
                 e.Write(name, values.data());
             })
        .def("InquireVariable",
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
             })
        .def("Close", &Engine::Close);


    // 
    // Wrappings to extend Engine in Python
    //

    // This first base class used to translate the protected template functions
    // into public members that can be overridden in python
    class PyEngineBase : public Engine
    {
    public:
        PyEngineBase(const std::string engineType, IO &io,
                     const std::string &name, const OpenMode openMode)
        : Engine(engineType, io, name, openMode, io.m_MPIComm)
        {
        }

        virtual ~PyEngineBase() = default;

        using Engine::Init;
        virtual void DoWrite(VariableBase &var, pybind11::array values) = 0;
        using Engine::Close;

    protected:
        // The C++ code calls this implementation which will redirect to the
        // python implementation.
#define define_dowrite(T)                                                      \
    void DoWrite(Variable<T> &var, const T *values) override                   \
    {                                                                          \
        DoWrite(var,                                                           \
                pybind11::array_t<T>(std::accumulate(var.m_Count.begin(),      \
                                                     var.m_Count.end(), 0),    \
                                     values));                                 \
    }
        ADIOS2_FOREACH_TYPE_1ARG(define_dowrite)
#undef define_dowrite
    };


    // This is the trampoline class used by pybind11 to implement the
    // inheritance model and virtual springboard
    class PyEngine : public PyEngineBase
    {
    public:
        using PyEngineBase::PyEngineBase;
        void Init() override
        {
          PYBIND11_OVERLOAD(void, PyEngineBase, Init,);
        }
        void DoWrite(VariableBase &var, pybind11::array values) override
        {
            PYBIND11_OVERLOAD_PURE(void, PyEngineBase, DoWrite, var, values);
        }
        void Close(const int transportIndex)
        {
            PYBIND11_OVERLOAD_PURE(void, PyEngineBase, Close, transportIndex);
        }
    };

    pybind11::class_<PyEngineBase, PyEngine>(m, "Engine")
        .def(pybind11::init<const std::string, IO &, const std::string,
                            const OpenMode>())
        .def("Init", &PyEngineBase::Init)
        .def("DoWrite",
             (void (PyEngineBase::*)(VariableBase &, pybind11::array)) &
                 PyEngineBase::DoWrite)
        .def("Close", &PyEngineBase::Close);
}
} // end namespace adios2
