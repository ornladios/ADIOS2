/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PythonEngine.h Support for an engine implemented in python
 *
 *  Created on: Sept 21, 2017
 *      Author: Scott Wittenburg <scott.wittenburg@kitware.com>
 */

#ifndef ADIOS2_ENGINE_PYENGINE_PYTHONENGINE_H_
#define ADIOS2_ENGINE_PYENGINE_PYTHONENGINE_H_


#include <functional>  // for function
#include <memory>      // for unique_ptr
#include <string>      // for string
#include <type_traits> // for add_pointer
#include <vector>      // for vector

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios2
{

/** A front-end wrapper for an engine implemented in python */
class PythonEngine : public Engine
{

public:
    PythonEngine(IO &io, const std::string &name, const OpenMode openMode,
                 MPI_Comm mpiComm);
    virtual ~PythonEngine();

    void PerformReads(ReadMode mode) override;
    void Release() override;
    void Advance(const float timeoutSeconds = 0.0) override;
    void Advance(const AdvanceMode mode,
                 const float timeoutSeconds = 0.0) override;
    void AdvanceAsync(const AdvanceMode mode,
                      AdvanceAsyncCallback callback) override;

    void SetCallBack(std::function<void(const void *, std::string, std::string,
                                        std::string, std::vector<size_t>)>
                         callback) override;

    void Close(const int transportIndex = -1) override;

protected:
    void Init() override;

#define declare(T)                                                             \
    void DoWrite(Variable<T> &variable, const T *values) override;             \
    void DoScheduleRead(Variable<T> &variable, const T *values) override;      \
    void DoScheduleRead(const std::string &variableName, const T *values)      \
        override;
    ADIOS2_FOREACH_TYPE_1ARG(declare)
#undef declare
    void DoWrite(VariableCompound &variable, const void *values) override;

#define declare(T, L)                                                          \
    Variable<T> *InquireVariable##L(const std::string &name,                   \
                                    const bool readIn) override;
    ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare
    VariableBase *InquireVariableUnknown(const std::string &name,
                                         const bool readIn) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_PYENGINE_PYTHONENGINE_H_ */
