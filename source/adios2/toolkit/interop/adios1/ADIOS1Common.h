/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Common.h
 *
 *  Created on: Jun 1, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_H_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_H_

// Fake out the include guard from ADIOS1's mpidummy.h to prevent it from
// getting included
#ifdef _NOMPI
#define __MPI_DUMMY_H__
#define MPI_Comm int
#endif
#include <adios.h>
#ifdef _NOMPI
#undef MPI_Comm
#undef __MPI_DUMMY_H__
#endif

#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios
{
namespace interop
{

class ADIOS1Common
{
public:
    /** ADIOS1 unique group name created from IO class object name */
    const std::string m_GroupName;

    /**Save file name from constructor for Advance when re-open in ADIOS1 */
    const std::string m_FileName;

    const std::string m_OpenModeString;

    bool m_IsInitialized = false; ///< set to true after calling adios_init()
    int64_t m_ADIOSFile = 0;  ///< ADIOS1 file handler returned by adios_open()
    int64_t m_ADIOSGroup = 0; ///< ADIOS1 group pointer that holds the ADIOS1
                              /// variable definitions
    bool m_IsFileOpen = false;

    int m_VerboseLevel = 0;
    ADIOS_ERRCODES m_ErrorNumber = static_cast<ADIOS_ERRCODES>(-1);

    ADIOS1Common(const std::string &groupName, const std::string &fileName,
                 const OpenMode openMode, MPI_Comm mpiComm,
                 const bool debugMode);

    ~ADIOS1Common();

    void InitParameters(const Params &parameters);
    void InitTransports(const std::vector<Params> &transportsParameters);

    template <class T>
    void WriteVariable(const std::string &name, const ShapeID shapeID,
                       const Dims ldims, const Dims gdims, const Dims offsets,
                       const T *values);

    void Advance();

    void Close();

private:
    MPI_Comm m_MPIComm = MPI_COMM_SELF;
    const bool m_DebugMode = false;

    void Init();

    bool ReOpenAsNeeded(); // return true if file is open or reopened

    void DefineVariable(const std::string &name, const ShapeID shapeID,
                        enum ADIOS_DATATYPES vartype, const std::string ldims,
                        const std::string gdims, const std::string offsets);

    template <class T>
    enum ADIOS_DATATYPES GetADIOS1Type() const;

    std::string DimsToCSVLocalAware(const Dims &dims);
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void ADIOS1Common::WriteVariable<T>(                       \
        const std::string &name, const ShapeID shapeID, const Dims ldims,      \
        const Dims gdims, const Dims offsets, const T *values);
ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMON_H_ */
