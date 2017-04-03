/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableBase.h
 *
 *  Created on: Feb 20, 2017
 *      Author: wfg
 */

#ifndef VARIABLEBASE_H_
#define VARIABLEBASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
/// \endcond

#include "functions/adiosFunctions.h" //GetTotalSize
#include "functions/adiosTemplates.h" //GetType<T>

namespace adios
{

using Dims = std::vector<std::size_t>;

class VariableBase
{

public:
  const std::string m_Name;        ///< variable name
  const std::string m_Type;        ///< variable type
  const std::size_t m_ElementSize; ///< Variable -> sizeof(T), VariableCompound
                                   ///-> from constructor

  bool m_IsScalar = false;
  const bool m_IsDimension = false;

  VariableBase(const std::string name, const std::string type,
               const std::size_t elementSize, const Dims dimensions,
               const Dims globalDimensions, const Dims globalOffsets,
               const bool debugMode)
  : m_Name{name}, m_Type{type}, m_ElementSize{elementSize},
    m_Dimensions{dimensions}, m_GlobalDimensions{globalDimensions},
    m_GlobalOffsets{globalOffsets}, m_DebugMode{debugMode}
  {
  }

  virtual ~VariableBase() {}

  std::size_t DimensionsSize() const noexcept { return m_Dimensions.size(); }

  /**
   * Returns the payload size in bytes
   * @return TotalSize * sizeof(T)
   */
  std::size_t PayLoadSize() const noexcept
  {
    return GetTotalSize(m_Dimensions) * m_ElementSize;
  }

  /**
   * Returns the total size
   * @return number of elements
   */
  std::size_t TotalSize() const noexcept { return GetTotalSize(m_Dimensions); }

  // protected: off for now

  Dims m_Dimensions;       ///< array of local dimensions
  Dims m_GlobalDimensions; ///< array of global dimensions
  Dims m_GlobalOffsets;    ///< array of global offsets
  const bool m_DebugMode = false;

  std::string GetDimensionAsString() { return dimsToString(m_Dimensions); }
  std::string GetGlobalDimensionAsString()
  {
    return dimsToString(m_GlobalDimensions);
  }
  std::string GetOffsetsAsString() { return dimsToString(m_GlobalOffsets); }

private:
  std::string dimsToString(Dims dims)
  {
    std::ostringstream oss;
    if (!dims.empty())
    {
      // Convert all but the last element to avoid a trailing ","
      std::copy(dims.begin(), dims.end() - 1,
                std::ostream_iterator<std::size_t>(oss, ","));
      // Now add the last element with no delimiter
      oss << dims.back();
    }
    return oss.str();
  }
};
}

#endif /* VARIABLEBASE_H_ */
