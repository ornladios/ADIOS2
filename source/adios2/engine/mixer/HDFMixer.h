/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDFMixer.h
 *
 *  Created on: Aug 16 2017
 *      Author: Junmin GU
 */

#ifndef ADIOS2_ENGINE_H5_HDFMIXER_H_
#define ADIOS2_ENGINE_H5_HDFMIXER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
//#include "adios2/toolkit/format/bp1/BP1Writer.h" //format::BP1Writer

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::count, std::copy, std::for_each
#include <cmath>     //std::ceil
#include <cstring>   //std::memcpy
/// \endcond

#include "HDFMixerWriter.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h"
//#include "adios2/toolkit/capsule/heap/STLVector.h"
#include "adios2/toolkit/transportman/TransportMan.h" //transport::TransportsMan

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

class HDFMixer : public Engine
{

public:
    /**
     * Constructor for file Writer in H5 format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param comm multi-process communicator
     */
    HDFMixer(IO &io, const std::string &name, const Mode openMode,
             helper::Comm comm);

    ~HDFMixer();

    // void Advance(const float timeoutSeconds =
    // std::numeric_limits<float>::max()) final;
    StepStatus BeginStep(StepMode mode, const float timeout_sec);
    // void EndStep(const float /*timeout_sec*/);
    void EndStep() final;

    void PerformPuts() final;

    void CreateName(std::string &pathName, std::string &rootName,
                    std::string &fullH5Name, int rank);

private:
    /** Single object controlling H5 buffering */
    // format::H51Writer m_H51Writer;
    HDFSerialWriter m_HDFSerialWriter;
    HDFVDSWriter m_HDFVDSWriter;

    /** single object controlling a vector of Transports from IO AddTransport */
    transportman::TransportMan m_TransportsManager;

    /** true: due to buffer overflow, move to transports manager */
    bool m_DoTransportFlush = false;

    void Init() final;

    /** Parses parameters from IO SetParameters */
    void InitParameters() final;
    /** Parses transports and parameters from IO AddTransport */
    void InitTransports() final;

    void InitBuffer();

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &variable, const T *values) /*final */;         \
    void DoPutDeferred(Variable<T> &variable, const T *values) /*final */;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports,
     * otherwise it
     * closes a transport in m_Transport[transportIndex]. In debug mode the
     * latter is bounds-checked.
     */
    void DoClose(const int transportIndex = -1) final;

    /**
     * Common function for primitive (including std::complex) writes
     * @param variable
     * @param values
     */
    template <class T>
    void DoPutSyncCommon(Variable<T> &variable, const T *values);

    /** Write a profiling.json file from m_H51Writer and m_TransportsManager
     * profilers*/
    void WriteProfilingJSONFile();
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_H5_HDFMIXER_H_ */
