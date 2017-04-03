/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FStream.h
 *
 *  Created on: Oct 18, 2016
 *      Author: wfg
 */

#ifndef FSTREAM_H_
#define FSTREAM_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <fstream>
/// \endcond

#include "core/Transport.h"

namespace adios
{
namespace transport
{

/**
 * File stream transport using C++ fstream
 */
class FStream : public Transport
{

public:
  FStream(MPI_Comm mpiComm, const bool debugMode);

  virtual ~FStream() = default;

  void Open(const std::string name, const std::string accessMode);

  void SetBuffer(char *buffer, std::size_t size);

  void Write(const char *buffer, std::size_t size);

  void Flush();

  void Close();

private:
  std::fstream m_FStream; ///< file stream under name.bp.dir/name.bp.rank
};

} // end namespace transport
} // end namespace adios

#endif /* FSTREAM_H_ */
