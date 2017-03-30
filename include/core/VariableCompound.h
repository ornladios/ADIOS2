/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableCompound.h
 *
 *  Created on: Feb 20, 2017
 *      Author: wfg
 */

#ifndef VARIABLECOMPOUND_H_
#define VARIABLECOMPOUND_H_

#include "core/VariableBase.h"

namespace adios
{

struct CompoundElement
{
  const std::string m_Name;
  const std::size_t m_Offset;
  const std::string m_Type;
};

/**
 * @param Base (parent) class for template derived (child) class CVariable.
 * Required to put CVariable objects in STL containers.
 */
class VariableCompound : public VariableBase
{

public:
  const void *m_AppValue = nullptr;

  VariableCompound(const std::string name, const std::size_t sizeOfStruct,
                   const Dims dimensions, const Dims globalDimensions,
                   const Dims globalOffsets, const bool debugMode)
      : VariableBase(name, "compound", sizeOfStruct, dimensions,
                     globalDimensions, globalOffsets, debugMode)
  {
  }

  template <class U>
  void InsertMember(const std::string name, const std::size_t offset)
  {
    m_Elements.push_back(CompoundElement{name, offset, GetType<U>()});
  }

  void Monitor(std::ostream &logInfo) const noexcept
  {
    logInfo << "Variable Compound: " << m_Name << "\n";
    logInfo << "Type: " << m_Type << "\n";
    logInfo << "Size: " << TotalSize() << " elements\n";
    logInfo << "Payload: " << PayLoadSize() << " bytes\n";
  }

private:
  std::vector<CompoundElement> m_Elements; ///< vector of element types
};

} // end namespace

#endif /* VARIABLECOMPOUND_H_ */
