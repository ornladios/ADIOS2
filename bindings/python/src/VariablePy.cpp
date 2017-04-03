/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariablePy.cpp
 *
 *  Created on: Mar 17, 2017
 *      Author: wfg
 */

#include "VariablePy.h"

namespace adios
{

VariablePy::VariablePy(const std::string name, const pyList localDimensionsPy,
                       const pyList globalDimensionsPy,
                       const pyList globalOffsetsPy)
    : m_Name{name}, m_LocalDimensions{ListToVector(localDimensionsPy)},
      m_GlobalDimensions{ListToVector(globalDimensionsPy)},
      m_GlobalOffsets{ListToVector(globalOffsetsPy)}
{
}

VariablePy::~VariablePy() {}

void VariablePy::SetLocalDimensions(const pyList list)
{
  //      this->m_Dimensions = ListToVector( list );
}

void VariablePy::SetGlobalDimensionsAndOffsets(const pyList globalDimensions,
                                               const pyList globalOffsets)
{
  //        this->m_GlobalDimensions = ListToVector( globalDimensions );
  //        this->m_GlobalOffsets = ListToVector( globalOffsets );
}

Dims VariablePy::GetLocalDimensions() { return this->m_LocalDimensions; }

} // end namespace
