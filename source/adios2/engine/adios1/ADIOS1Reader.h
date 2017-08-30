/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.h
 * Class to read files using old adios 1.x library.
 * It requires adios 1.x installed
 *
 *  Created on: Mar 27, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

// Fake out the include guard from ADIOS1's mpidummy.h to prevent it from
// getting included
#ifdef _NOMPI
#define __MPI_DUMMY_H__
#define MPI_Comm int
#endif
#include <adios_read_v2.h>
#ifdef _NOMPI
#undef MPI_Comm
#undef __MPI_DUMMY_H__
#endif

namespace adios2
{

class ADIOS1Reader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param hostLanguage
     */
    ADIOS1Reader(IO &adios, const std::string &name, const Mode openMode,
                 MPI_Comm mpiComm);

    ~ADIOS1Reader();

    void PerformReads(ReadMode mode);

    void Release();
    void Advance(const float timeout_sec = 0.0);
    void Advance(AdvanceMode mode, const float timeout_sec = 0.0);
    void
    AdvanceAsync(AdvanceMode mode,
                 std::function<void(std::shared_ptr<adios2::Engine>)> callback);

    void Close(const int transportIndex = -1);

private:
    ADIOS_FILE *m_fh = nullptr; ///< ADIOS1 file handler
    bool m_OpenAsFile = false;

    void Init() final; ///< called from constructor, gets the selected ADIOS1
                       /// transport method from settings
    void InitParameters() final;
    void InitTransports() final;

#define declare(T, L)                                                          \
    virtual Variable<T> *DoInquireVariable##L(const std::string &variableName) \
        final;

    ADIOS2_FOREACH_TYPE_2ARGS(declare)
#undef declare

    /** Not yet implemented */
    VariableCompound *InquireVariableCompound(const std::string &name,
                                              const bool readIn);

    template <class T>
    Variable<T> *InquireVariableCommon(const std::string &name);

#define declare_type(T)                                                        \
    void DoScheduleRead(Variable<T> &variable, const T *values) final;         \
    void DoScheduleRead(const std::string &variableName, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    void ScheduleReadCommon(const std::string &name, const Dims &offs,
                            const Dims &ldims, const int fromStep,
                            const int nSteps, const bool readAsLocalValue,
                            const bool readAsJoinedArray, void *data);

    void ReadJoinedArray(const std::string &name, const Dims &offs,
                         const Dims &ldims, const int fromStep,
                         const int nSteps, void *data);

    bool CheckADIOS1TypeCompatibility(const std::string &name,
                                      std::string adios2Type,
                                      enum ADIOS_DATATYPES adios1Type);

    enum ADIOS_READ_METHOD m_ReadMethod = ADIOS_READ_METHOD_BP;
};

} // end namespace adios

#include "ADIOS1Reader.inl"

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_ */
