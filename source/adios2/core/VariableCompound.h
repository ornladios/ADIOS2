/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableCompound.h
 *
 *  Created on: Feb 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_VARIABLECOMPOUND_H_
#define ADIOS2_CORE_VARIABLECOMPOUND_H_

#include "VariableBase.h"

#include "adios2/ADIOSMacros.h"

namespace adios
{

/**
 * @param Base (parent) class for template derived (child) class CVariable.
 * Required to put CVariable objects in STL containers.
 */
class VariableCompound : public VariableBase
{

public:
    const void *m_AppValue = nullptr;

    /** Primitive type element */
    struct Element
    {
        const std::string m_Name;
        const std::string m_Type; ///< from GetType<T>
        const size_t m_Offset;    ///< element offset in struct
    };

    /** vector of primitve element types defining compound struct */
    std::vector<Element> m_Elements;

    VariableCompound(const std::string name, const std::size_t sizeOfStruct,
                     const Dims shape, const Dims start, const Dims count,
                     const bool constantShape, const bool debugMode);

    ~VariableCompound() = default;

    void ApplyTransforms() final;

    /**
     * Inserts an Element into the compound struct definition
     * @param name
     * @param offset
     */
    template <class T>
    void InsertMember(const std::string name, const size_t offset);
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void VariableCompound::InsertMember<T>(const std::string,  \
                                                           const size_t);
ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios

#endif /* ADIOS2_CORE_VARIABLECOMPOUND_H_ */
