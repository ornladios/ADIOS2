/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Variable.h : template class for self-describing primitive variables
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy
 */

#ifndef ADIOS2_CORE_VARIABLE_H_
#define ADIOS2_CORE_VARIABLE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <ostream> //std::ostream in MonitorGroups
#include <string>
#include <unordered_map>
#include <vector>
/// \endcond

#include "adios2/ADIOSTypes.h"
#include "adios2/core/VariableBase.h"

namespace adios2
{
namespace core
{

/**
 * @param Base (parent) class for template derived (child) class Variable.
 */
template <class T>
class Variable : public VariableBase
{

public:
    /** current reference to data */
    T *m_Data = nullptr;
    /** absolute minimum */
    T m_Min = T();
    /** absolute maximum */
    T m_Max = T();
    /** current value */
    T m_Value = T();

    struct Info
    {
        Dims Shape;
        Dims Start;
        Dims Count;
        T *Data = nullptr;

        T Min = T();
        T Max = T();
        T Value = T();
    };

    std::unordered_map<size_t, std::map<size_t, Info>> m_StepBlocksInfo;

    Variable<T>(const std::string &name, const Dims &shape, const Dims &start,
                const Dims &count, const bool constantShape,
                const bool debugMode);

    ~Variable<T>() = default;

    Info &SetStepBlockInfo(const T *data, const size_t step) noexcept;

    void SetData(const T *data) noexcept;

    T *GetData() const noexcept;
};

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_H_ */
