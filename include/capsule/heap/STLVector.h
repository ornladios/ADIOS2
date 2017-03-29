/*
 * Heap.h
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#ifndef STLVECTOR_H_
#define STLVECTOR_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
/// \endcond

#include "core/Capsule.h"

namespace adios
{
namespace capsule
{

/**
 * Data and Metadata buffers are allocated in the Heap
 */
class STLVector : public Capsule
{

public:
  std::vector<char> m_Data;     ///< data buffer allocated using the STL in heap
                                /// memory, default size = 16 Mb
  std::vector<char> m_Metadata; ///< metadata buffer allocated using the STL in
                                /// heap memory, default size = 100 Kb

  /**
   * Unique constructor
   * @param accessMode read, write or append
   * @param rankMPI MPI rank
   * @param debugMode true: extra checks, slower
   */
  STLVector(const std::string accessMode, const int rankMPI,
            const bool debugMode = false);

  ~STLVector();

  char *GetData();
  char *GetMetadata();

  std::size_t GetDataSize() const;
  std::size_t GetMetadataSize() const;

  void ResizeData(const std::size_t size);
  void ResizeMetadata(const std::size_t size);
};

} // end namespace capsule
} // end namespace

#endif /* STLVECTOR_H_ */
