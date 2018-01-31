/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.h
 *
 *  Created on: Feb 21, 2017
 *      Author: Jason Wang
 *              William F Godoy
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/dataman/DataMan.h"

#include <json.hpp>

namespace adios2
{

class DataManReader : public Engine
{

public:
    /**
     * Constructor for dataman engine Reader for WAN communications
     * @param adios
     * @param name
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param nthreads
     */
    DataManReader(IO &io, const std::string &name, const Mode mode,
                  MPI_Comm mpiComm);

    virtual ~DataManReader();

    StepStatus BeginStep(StepMode stepMode,
                         const float timeoutSeconds = 0.f) final;

    void PerformGets() final;

    void EndStep() final;
    size_t CurrentStep() const;

private:
    unsigned int m_nDataThreads = 1;
    unsigned int m_nControlThreads = 0;
    unsigned int m_TransportChannels = 1;
    size_t m_BufferSize = 1024 * 1024 * 1024;
    std::string m_UseFormat = "json";
    bool m_DoMonitor = false;
    bool m_Blocking = true;

    std::shared_ptr<transportman::DataMan> m_DataMan;
    std::vector<std::thread> m_DataThreads;

    std::shared_ptr<transportman::DataMan> m_ControlMan;
    std::vector<std::thread> m_ControlThreads;

    std::vector<adios2::Operator *> m_Callbacks;

    // The current time step that the reader app is reading
    size_t m_CurrentStep = 0;

    // The oldest time step contained in m_VariableMap
    size_t m_OldestStep = 0;

    bool m_Listening = false;

    struct DataManVar
    {
        std::vector<char> data;
        std::string type;
        Dims shape;
        Dims start;
        Dims count;
        size_t size;
    };

    std::unordered_map<
        size_t, std::unordered_map<std::string, std::shared_ptr<DataManVar>>>
        m_VariableMap;

    std::mutex m_MutexIO;
    std::mutex m_MutexMap;

    void ReadThread(std::shared_ptr<transportman::DataMan> man);

    void Init();

    void RunCallback(void *buffer, std::string doid, std::string var,
                     std::string dtype, std::vector<size_t> shape);
    bool GetBoolParameter(Params &params, std::string key, bool &value);
    bool GetStringParameter(Params &params, std::string key,
                            std::string &value);
    bool GetUIntParameter(Params &params, std::string key, unsigned int &value);

    void DoClose(const int transportIndex = -1) final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    void DoGetDeferred(Variable<T> &, T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * All GetSync virtual functions call this function
     * @param variable
     * @param data
     */
    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_ */
