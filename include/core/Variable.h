/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.h
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <ostream> //std::ostream in MonitorGroups
#include <string>
#include <vector>
/// \endcond

#include "core/Transform.h"
#include "core/VariableBase.h"

namespace adios
{

struct TransformData
{
    Transform &Operation; ///< pointer to transform object
    std::map<std::string, std::string> Parameters; ///< transforms parameters
    std::vector<std::size_t> Size; ///< vector that carries the sizes after a
                                   /// transformation is applied
};

/**
 * @param Base (parent) class for template derived (child) class CVariable.
 * Required to put CVariable objects in STL containers.
 */
template <class T> class Variable : public VariableBase
{

public:
    const T *m_AppValues = nullptr; ///< pointer to values passed from user in
    /// ADIOS Write, it might change in ADIOS Read

    std::vector<TransformData>
        m_Transforms; ///< associated transforms, sequence
    /// determines application order, e.g.
    /// first Transforms[0] then
    /// Transforms[1]. Pointer used as
    /// reference (no memory management).

    Variable<T>(const std::string name, const Dims dimensions,
                const Dims globalDimensions, const Dims globalOffsets,
                const bool debugMode)
    : VariableBase(name, GetType<T>(), sizeof(T), dimensions, globalDimensions,
                   globalOffsets, debugMode)
    {
        if (m_Dimensions == Dims{1})
            m_IsScalar = true;
    }

    template <class... Args>
    void AddTransform(Transform &transform, Args... args)
    {
        std::vector<std::string> parameters = {args...};
        m_Transforms.emplace_back(
            transform,
            BuildParametersMap(parameters, m_DebugMode)); // need to check
    }

    /** Return the global dimensions of the variable
     *  @return vector of std::size_t values
     */
    std::vector<std::size_t> GetGlobalDimensions();

    /** Return the number of steps available for the variable
     *  @return Number of steps
     */
    int GetSteps();

    void Monitor(std::ostream &logInfo) const noexcept
    {
        logInfo << "Variable: " << m_Name << "\n";
        logInfo << "Type: " << m_Type << "\n";
        logInfo << "Size: " << TotalSize() << " elements\n";
        logInfo << "Payload: " << PayLoadSize() << " bytes\n";

        if (m_AppValues != nullptr)
        {
            logInfo << "Values (first 10 or max_size): \n";
            std::size_t size = TotalSize();
            if (size > 10)
                size = 10;

            if (m_Type.find("complex") != m_Type.npos) // it's complex
            {
                for (unsigned int i = 0; i < size; ++i)
                {
                    logInfo << "( " << std::real(m_AppValues[i]) << " , "
                            << std::imag(m_AppValues[i]) << " )  ";
                }
            }
            else
            {
                for (unsigned int i = 0; i < size; ++i)
                {
                    logInfo << m_AppValues[i] << " ";
                }
            }

            logInfo << " ...";
        }
        logInfo << "\n";
    }
};

} // end namespace

#endif /* VARIABLE_H_ */
