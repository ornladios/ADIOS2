/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1CommonRead.h
 *
 *  Created on: Oct 26, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONREAD_H_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONREAD_H_

// Fake out the include guard from ADIOS1's mpidummy.h to prevent it from
// getting included
#ifdef _NOMPI
#define __MPI_DUMMY_H__
#define MPI_Comm int
#endif
#include <adios_error.h>
#include <adios_read.h>
#ifdef _NOMPI
#undef MPI_Comm
#undef __MPI_DUMMY_H__
#endif

#include "ADIOS1Common.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"

namespace adios2
{
namespace interop
{

class ADIOS1CommonRead : public ADIOS1Common
{
public:
    ADIOS1CommonRead(const std::string &fileName, MPI_Comm mpiComm,
                     const bool debugMode);

    ~ADIOS1CommonRead();

    void InitParameters(const Params &parameters);
    void InitTransports(const std::vector<Params> &transportsParameters);
    bool Open(core::IO &io); // return true if file is opened successfully

    void ScheduleReadCommon(const std::string &name, const Dims &offs,
                            const Dims &ldims, const int fromStep,
                            const int nSteps, const bool readAsLocalValue,
                            const bool readAsJoinedArray, void *data);

    void PerformReads();
    StepStatus AdvanceStep(core::IO &io, const StepMode mode,
                           const float timeout_sec = 0.0);
    size_t CurrentStep() const;
    void ReleaseStep();

    ADIOS_VARINFO *InqVar(const std::string &varName);
    void InqVarBlockInfo(ADIOS_VARINFO *vi);
    void FreeVarInfo(ADIOS_VARINFO *vi);

    bool IsVarLocalValue(ADIOS_VARINFO *vi);

    void Close();

private:
    enum ADIOS_READ_METHOD m_ReadMethod;
    bool m_OpenAsFile = false;
    ADIOS_FILE *m_fh = nullptr; ///< ADIOS1 file handler

    // In streaming mode, Open() does not generate the variable map.
    // The first BeginStep() call does that to publish the variables of the
    // first step
    bool m_IsBeforeFirstStep = true;

    void Init();

    void DefineADIOS2Variable(core::IO &io, const char *name,
                              const ADIOS_VARINFO *vi, Dims gdims,
                              bool isJoined, bool isGlobal);

    template <class T>
    void DefineADIOS2Variable(core::IO &io, const char *name,
                              const ADIOS_VARINFO *vi, Dims gdims,
                              bool isJoined, bool isGlobal);

    void GenerateVariables(core::IO &io);

    void DefineADIOS2Attribute(core::IO &io, const char *name,
                               enum ADIOS_DATATYPES type, void *value);

    template <class T>
    void DefineADIOS2Attribute(core::IO &io, const char *name, void *value);

    void GenerateAttributes(core::IO &io);
};

} // end namespace interop
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONREAD_H_ */
