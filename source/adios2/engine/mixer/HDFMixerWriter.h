/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDFMixer.h
 *
 *  Created on: Aug 16 2017
 *      Author: Junmin GU
 */

#ifndef ADIOS2_ENGINE_BP_HDFSERIALWRITER_H_
#define ADIOS2_ENGINE_BP_HDFSERIALWRITER_H_

#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/interop/hdf5/HDF5Common.h"

namespace adios2
{
namespace core
{
namespace engine
{

class HDFVDSWriter
{
public:
    HDFVDSWriter(helper::Comm const &comm, bool debugMode);
    void Init(const std::string &name);
    void AddVar(const VariableBase &var, hid_t h5Type);
    void
    Advance(const float timeoutSeconds = std::numeric_limits<float>::max());
    void Close(const int transportIndex = -1);

    interop::HDF5Common m_VDSFile;
    int m_Rank;

private:
    void GetVarInfo(const VariableBase &var, std::vector<hsize_t> &dimsf,
                    int nDim, std::vector<hsize_t> &start,
                    std::vector<hsize_t> &count, std::vector<hsize_t> &one);

    int m_NumSubFiles;
    std::string m_FileName;
    helper::Comm const
        &m_SubfileComm; // only rank 0 in this comm can build VDS;
};

class HDFSerialWriter
{
public:
    HDFSerialWriter(helper::Comm const &comm, bool debugMode);
    void
    Advance(const float timeoutSeconds = std::numeric_limits<float>::max());
    void Close(const int transportIndex = -1);
    void Init(const std::string &name, int rank);

    static void StaticCreateName(std::string &pathName, std::string &rootName,
                                 std::string &fullH5Name,
                                 const std::string &input, int rank);
    /** contains data buffer and position */
    // capsule::STLVector m_HeapBuffer;

    // int m_Rank;
    interop::HDF5Common m_H5File;
    std::string m_FileName;

private:
    helper::Comm const
        &m_LocalComm; // all ranks in this comm write to the same file
    const bool m_DebugMode = false;
    int m_Rank;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_BP_HDFSerialWriter
