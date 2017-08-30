/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_

#include <iostream> //std::cout must be removed, only used for hello example
#include <unistd.h> //sleep must be removed

#include <DataMan.h>

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

namespace adios2
{

class DataManWriter : public Engine
{

public:
    using json = nlohmann::json;

    DataManWriter(IO &io, const std::string &name, const Mode openMode,
                  MPI_Comm mpiComm);

    virtual ~DataManWriter() = default;

    void SetCallBack(std::function<void(const void *, std::string, std::string,
                                        std::string, Dims)>
                         callback) final;

    void Advance(const float timeoutSeconds = 0.0) final;

    void Close(const int transportIndex = -1) final;

private:
    bool m_DoRealTime = false;
    bool m_DoMonitor = false;
    DataMan m_Man;
    std::function<void(const void *, std::string, std::string, std::string,
                       Dims)>
        m_CallBack; ///< call back function

    void Init(); ///< calls InitCapsules and InitTransports based on Method,
                 /// called from constructor

#define declare_type(T)                                                        \
    void DoWrite(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void DoWriteCommon(Variable<T> &variable, const T *values);
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
