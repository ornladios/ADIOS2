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
#include <vector>
/// \endcond

#include "adios2/core/VariableBase.h"

namespace adios2
{

/**
 * @param Base (parent) class for template derived (child) class Variable.
 */
template <class T>
class Variable : public VariableBase
{

public:
    /**
     * Unique constructor
     * @param name
     * @param shape
     * @param start
     * @param count
     * @param constantShape
     * @param debugMode
     */
    Variable<T>(const std::string &name, const Dims &shape, const Dims &start,
                const Dims &count, const bool constantShape, T *data,
                const bool debugMode);

    ~Variable<T>() = default;

    T *GetData() const noexcept;

    void SetData(const T *) noexcept;

private:
    /** TODO: used for allocating memory from ADIOS2 */
    std::vector<T> m_AllocatedData;

    /** reference to non-const data */
    T *m_Data = nullptr;
};

} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLE_H_ */
