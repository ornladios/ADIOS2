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

// FIXME, VariableCompound is now also an alias for Variable<Compound>, so
// arguably this should be all merged into Variable.{h,tcc,cpp}
#include "Variable.h"

#include "adios2/ADIOSMacros.h"

namespace adios2
{
namespace core
{

/**
 * @param Base (parent) class for template derived (child) class CVariable.
 * Required to put CVariable objects in STL containers.
 */
template <>
class Variable<Compound> : public VariableBase
{

public:
    const void *m_Data = nullptr;

    /** Primitive type element */
    struct Element
    {
        const std::string Name;
        const std::string Type; ///< from GetType<T>
        const size_t Offset;    ///< element offset in struct
    };

    /** vector of primitve element types defining compound struct */
    std::vector<Element> m_Elements;

    Variable();
    Variable(const std::string &name, const size_t structSize,
             const Dims &shape, const Dims &start, const Dims &count,
             const bool constantDims, const bool debugMode);

    ~Variable() = default;

    /**
     * Inserts an Element into the compound struct definition
     * @param name
     * @param offset
     */
    template <class T>
    void InsertMember(const std::string &name, const size_t offset);
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void Variable<Compound>::InsertMember<T>(                  \
        const std::string &, const size_t);
ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

using VariableCompound = Variable<Compound>;

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_VARIABLECOMPOUND_H_ */
