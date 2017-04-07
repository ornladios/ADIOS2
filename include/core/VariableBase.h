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

#include "functions/adiosFunctions.h" //GetTotalSize, DimsToCSV
#include "functions/adiosTemplates.h" //GetType<T>

namespace adios
{

using Dims = std::vector<std::size_t>;

class VariableBase
{

public:
    const std::string m_Name; ///< variable name
    const std::string m_Type; ///< variable type

    /**
     * Variable -> sizeof(T),
     * VariableCompound -> from constructor sizeof(struct)
     */
    const std::size_t m_ElementSize;

    Dims m_LocalDimensions;  ///< dimensions per rank (MPI)
    Dims m_GlobalDimensions; ///< total dimensions across MPI
    Dims m_Offsets;          ///< selections offset
    bool m_IsScalar = false;
    const bool m_IsDimension = false;

    VariableBase(const std::string name, const std::string type,
                 const std::size_t elementSize, const Dims localDimensions,
                 const Dims globalDimensions, const Dims offsets,
                 const bool debugMode)
    : m_Name{name}, m_Type{type}, m_ElementSize{elementSize},
      m_LocalDimensions{localDimensions}, m_GlobalDimensions{globalDimensions},
      m_Offsets{offsets}, m_DebugMode{debugMode}
    {
    }

    virtual ~VariableBase() {}

    std::size_t DimensionsSize() const noexcept
    {
        return m_LocalDimensions.size();
    }

    /**
     * Returns the payload size in bytes
     * @return TotalSize * sizeof(T)
     */
    std::size_t PayLoadSize() const noexcept
    {
        return GetTotalSize(m_LocalDimensions) * m_ElementSize;
    }

    /**
     * Returns the total size
     * @return number of elements
     */
    std::size_t TotalSize() const noexcept
    {
        return GetTotalSize(m_LocalDimensions);
    }

    // protected: off for now
    const bool m_DebugMode = false;
};
}

#endif /* VARIABLEBASE_H_ */
