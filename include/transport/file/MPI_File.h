/*
 * MPIFile.h
 *
 *  Created on: Jan 5, 2017
 *      Author: wfg
 */

#ifndef MPI_FILE_H_
#define MPI_FILE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <mpi.h>
/// \endcond

namespace adios
{
namespace transport
{

/**
 * Class that defines a transport method using C++ file streams
 */
class MPI_File : public Transport
{

public:
  MPI_File(MPI_Comm mpiComm, const bool debugMode);

  ~MPI_File();

  void Open(const std::string streamName, const std::string accessMode);

  void SetBuffer(char *buffer, std::size_t size);

  void Write(const char *buffer, std::size_t size);

  void Flush();

  void Close();

private:
  MPI_File m_MPIFile; ///< MPI File
};

} // end namespace transport
} // end namespace

#endif /* MPI_FILE_H_ */
