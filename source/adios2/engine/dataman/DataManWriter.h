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

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/dataman/DataMan.h"

namespace adios2
{

class DataManWriter : public Engine
{

public:
    DataManWriter(IO &io, const std::string &name, const Mode mode,
                  MPI_Comm mpiComm);

    ~DataManWriter() = default;

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void EndStep() final;

    void Close(const int transportIndex = -1) final;

private:
    format::BP3Serializer m_BP3Serializer;
    transportman::DataMan m_Man;
    std::string m_Name;

    unsigned int m_NChannels = 1;
    std::string m_UseFormat = "json";
    bool m_DoMonitor = false;

    void Init();
    void InitParameters();
    void InitTransports();

    bool GetBoolParameter(Params &params, std::string key, bool &value);
    bool GetStringParameter(Params &params, std::string key, std::string &value);
    bool GetUIntParameter(Params &params, std::string key, unsigned int &value);

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutSyncCommonBP(Variable<T> &variable, const T *values);
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
