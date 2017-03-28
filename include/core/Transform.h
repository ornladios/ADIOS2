/*
 * Transform.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef TRANSFORM_H_
#define TRANSFORM_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

namespace adios
{

/**
 * Parent class that defines data variable transformations. Used as a member of
 * CVariable
 */
class Transform
{

public:
  const std::string m_Method;

  /**
   * Initialize parent method
   * @param method zlib, bzip2, szip
   */
  Transform(const std::string method);

  virtual ~Transform();

  virtual void Compress(const std::vector<char> &bufferIn,
                        std::vector<char> &bufferOut);

  virtual void Decompress(const std::vector<char> &bufferIn,
                          std::vector<char> &bufferOut);
};

} // end namespace
#endif /* TRANSFORM_H_ */
