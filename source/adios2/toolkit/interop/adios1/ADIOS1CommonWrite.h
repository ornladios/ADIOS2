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

#ifndef ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONWRITE_H_
#define ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONWRITE_H_

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

#include "ADIOS1Common.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{
namespace interop
{

class ADIOS1CommonWrite : public ADIOS1Common
{
public:
    /** ADIOS1 unique group name created from IO class object name */
    const std::string m_GroupName;

    int64_t m_ADIOSFile = 0;  ///< ADIOS1 file handler returned by adios_open()
    int64_t m_ADIOSGroup = 0; ///< ADIOS1 group pointer that holds the ADIOS1
                              /// variable definitions

    ADIOS_ERRCODES m_ErrorNumber = static_cast<ADIOS_ERRCODES>(-1);

    ADIOS1CommonWrite(const std::string &groupName, const std::string &fileName,
                      MPI_Comm mpiComm, const bool debugMode);

    ~ADIOS1CommonWrite();

    void InitParameters(const Params &parameters);
    void InitTransports(const std::vector<Params> &transportsParameters);
    bool Open(const Mode openMode); // return true if file is opened
    bool ReOpenAsNeeded();          // return true if file is open or reopened

    template <class T>
    void WriteVariable(const std::string &name, const ShapeID shapeID,
                       const Dims ldims, const Dims gdims, const Dims offsets,
                       const T *values);

    void Advance();

    void Close();

private:
    void Init();

    void DefineVariable(const std::string &name, const ShapeID shapeID,
                        enum ADIOS_DATATYPES vartype, const std::string ldims,
                        const std::string gdims, const std::string offsets);
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void ADIOS1CommonWrite::WriteVariable<T>(                  \
        const std::string &name, const ShapeID shapeID, const Dims ldims,      \
        const Dims gdims, const Dims offsets, const T *values);
ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_INTEROP_ADIOS1_ADIOS1COMMONWRITE_H_ */
