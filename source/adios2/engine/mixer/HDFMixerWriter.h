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

#include "adios2/toolkit/interop/hdf5/HDF5Common.h"

/*
inline MPI_Datatype mpi_typeof(char) { return MPI_CHAR; }
inline MPI_Datatype mpi_typeof(signed short) { return MPI_SHORT; }
inline MPI_Datatype mpi_typeof(signed int) { return MPI_INT; }
inline MPI_Datatype mpi_typeof(signed long) { return MPI_LONG; }
inline MPI_Datatype mpi_typeof(unsigned char) { return MPI_UNSIGNED_CHAR; }
inline MPI_Datatype mpi_typeof(unsigned short) { return MPI_UNSIGNED_SHORT; }
inline MPI_Datatype mpi_typeof(unsigned) { return MPI_UNSIGNED; }
inline MPI_Datatype mpi_typeof(unsigned long) { return MPI_UNSIGNED_LONG; }
inline MPI_Datatype mpi_typeof(signed long long) { return MPI_LONG_LONG_INT; }
inline MPI_Datatype mpi_typeof(double) { return MPI_DOUBLE; }
inline MPI_Datatype mpi_typeof(long double) { return MPI_LONG_DOUBLE; }
inline MPI_Datatype mpi_typeof(std::pair<int, int>) { return MPI_2INT; }
inline MPI_Datatype mpi_typeof(std::pair<float, int>) { return MPI_FLOAT_INT; }
inline MPI_Datatype mpi_typeof(std::pair<double, int>)
{
    return MPI_DOUBLE_INT;
}
inline MPI_Datatype mpi_typeof(std::pair<long double, int>)
{
    return MPI_LONG_DOUBLE_INT;
}
inline MPI_Datatype mpi_typeof(std::pair<short, int>) { return MPI_SHORT_INT; }

#define ADIOS_MPI_SIZE_T (mpi_typeof(size_t()))
*/
namespace adios2
{
namespace core
{
namespace engine
{

class HDFVDSWriter
{
public:
    HDFVDSWriter(MPI_Comm mpiComm, bool debugMode);
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
    MPI_Comm m_MPISubfileComm; // only rank 0 in this comm can build VDS;
};

class HDFSerialWriter
{
public:
    HDFSerialWriter(MPI_Comm mpiComm, bool debugMode);
    void
    Advance(const float timeoutSeconds = std::numeric_limits<float>::max());
    void Close(const int transportIndex = -1);
    void Init(const std::string &name, int rank);

    static void StaticCreateName(std::string &pathName, std::string &rootName,
                                 std::string &fullH5Name,
                                 const std::string &input, int rank);
    /** contains data buffer and position */
    // capsule::STLVector m_HeapBuffer;

    // int m_MPIRank;
    interop::HDF5Common m_H5File;
    std::string m_FileName;

private:
    MPI_Comm m_MPILocalComm; // all ranks in this comm write to the same file
    const bool m_DebugMode = false;
    int m_Rank;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_BP_HDFSerialWriter
