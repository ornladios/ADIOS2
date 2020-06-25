/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.tcc template implementations with fix types and specializations
 *
 *  Created on: May 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_IO_TCC_
#define ADIOS2_CORE_IO_TCC_

#include "IO.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <iostream>
#include <memory>
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosType.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

namespace adios2
{
namespace core
{

template <class T>
Variable<T> &IO::DefineVariable(const std::string &name, const Dims &shape,
                                const Dims &start, const Dims &count,
                                const bool constantDims)
{
    TAU_SCOPED_TIMER("IO::DefineVariable");

    {
        auto itVariable = m_Variables.find(name);
        if (!IsEnd(itVariable, m_Variables))
        {
            throw std::invalid_argument("ERROR: variable " + name +
                                        " exists in IO object " + m_Name +
                                        ", in call to DefineVariable\n");
        }
    }

    const unsigned int newIndex =
        m_AttributeMap.empty() ? 0 : m_AttributeMap.rbegin()->first + 1;

    auto itVariablePair = m_VariableMap.emplace(
        newIndex, std::unique_ptr<VariableBase>(new Variable<T>(
                      name, shape, start, count, constantDims)));
    m_Variables.emplace(name,
                        std::make_pair(helper::GetDataType<T>(), newIndex));

    Variable<T> &variable =
        static_cast<Variable<T> &>(*itVariablePair.first->second);

    // check IO placeholder for variable operations
    auto itOperations = m_VarOpsPlaceholder.find(name);
    if (itOperations != m_VarOpsPlaceholder.end())
    {
        variable.m_Operations.reserve(itOperations->second.size());

        for (auto &operation : itOperations->second)
        {
            variable.AddOperation(*operation.Op, operation.Parameters);
        }
    }

    return variable;
}

template <class T>
Variable<T> *IO::InquireVariable(const std::string &name) noexcept
{
    TAU_SCOPED_TIMER("IO::InquireVariable");
    auto itVariable = m_Variables.find(name);

    if (itVariable == m_Variables.end())
    {
        return nullptr;
    }

    if (itVariable->second.first != helper::GetDataType<T>())
    {
        return nullptr;
    }

    Variable<T> *variable = static_cast<Variable<T> *>(
        m_VariableMap.at(itVariable->second.second).get());
    if (m_ReadStreaming)
    {
        if (!variable->IsValidStep(m_EngineStep + 1))
        {
            return nullptr;
        }
    }
    return variable;
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T &value,
                                  const std::string &variableName,
                                  const std::string separator)
{
    TAU_SCOPED_TIMER("IO::DefineAttribute");
    if (!variableName.empty() &&
        InquireVariableType(variableName) == DataType::None)
    {
        throw std::invalid_argument(
            "ERROR: variable " + variableName +
            " doesn't exist, can't associate attribute " + name +
            ", in call to DefineAttribute");
    }

    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    auto itExistingAttribute = m_Attributes.find(globalName);
    if (!IsEnd(itExistingAttribute, m_Attributes))
    {
        if (helper::ValueToString(value) ==
            m_AttributeMap.at(itExistingAttribute->second.second)
                ->GetInfo()["Value"])
        {
            return static_cast<Attribute<T> &>(
                *m_AttributeMap.at(itExistingAttribute->second.second));
        }
        else
        {
            throw std::invalid_argument(
                "ERROR: attribute " + globalName +
                " has been defined and its value cannot be changed, in call to "
                "DefineAttribute\n");
        }
    }
    const unsigned int newIndex =
        m_AttributeMap.empty() ? 0 : m_AttributeMap.rbegin()->first + 1;

    auto itAttributePair = m_AttributeMap.emplace(
        newIndex,
        std::unique_ptr<AttributeBase>(new Attribute<T>(globalName, value)));
    m_Attributes.emplace(globalName,
                         std::make_pair(helper::GetDataType<T>(), newIndex));

    return static_cast<Attribute<T> &>(*itAttributePair.first->second);
}

template <class T>
Attribute<T> &IO::DefineAttribute(const std::string &name, const T *array,
                                  const size_t elements,
                                  const std::string &variableName,
                                  const std::string separator)
{
    TAU_SCOPED_TIMER("IO::DefineAttribute");
    if (!variableName.empty() &&
        InquireVariableType(variableName) == DataType::None)
    {
        throw std::invalid_argument(
            "ERROR: variable " + variableName +
            " doesn't exist, can't associate attribute " + name +
            ", in call to DefineAttribute");
    }

    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    auto itExistingAttribute = m_Attributes.find(globalName);
    if (!IsEnd(itExistingAttribute, m_Attributes))
    {
        const std::string arrayValues(
            "{ " +
            helper::VectorToCSV(std::vector<T>(array, array + elements)) +
            " }");

        if (m_AttributeMap.at(itExistingAttribute->second.second)
                ->GetInfo()["Value"] == arrayValues)
        {
            return static_cast<Attribute<T> &>(
                *m_AttributeMap.at(itExistingAttribute->second.second));
        }
        else
        {
            throw std::invalid_argument(
                "ERROR: attribute " + globalName +
                " has been defined and its value cannot be changed, in call to "
                "DefineAttribute\n");
        }
    }
    const unsigned int newIndex =
        m_AttributeMap.empty() ? 0 : m_AttributeMap.rbegin()->first + 1;

    auto itAttributePair = m_AttributeMap.emplace(
        newIndex, std::unique_ptr<AttributeBase>(
                      new Attribute<T>(globalName, array, elements)));
    m_Attributes.emplace(globalName,
                         std::make_pair(helper::GetDataType<T>(), newIndex));

    return static_cast<Attribute<T> &>(*itAttributePair.first->second);
}

template <class T>
Attribute<T> *IO::InquireAttribute(const std::string &name,
                                   const std::string &variableName,
                                   const std::string separator) noexcept
{
    TAU_SCOPED_TIMER("IO::InquireAttribute");
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);
    auto itAttribute = m_Attributes.find(globalName);

    if (itAttribute == m_Attributes.end())
    {
        return nullptr;
    }

    if (itAttribute->second.first != helper::GetDataType<T>())
    {
        return nullptr;
    }

    return static_cast<Attribute<T> *>(
        m_AttributeMap.at(itAttribute->second.second).get());
}

// PRIVATE

template <class T>
Params IO::GetVariableInfo(const std::string &variableName,
                           const std::set<std::string> &keys)
{
    Params info;
    // keys input are case insensitive
    const std::set<std::string> keysLC = helper::LowerCase(keys);

    // return empty map if only "name" key is requested
    if (keys.size() == 1 && keysLC.count("name") == 1)
    {
        return info;
    }

    Variable<T> &variable = *InquireVariable<T>(variableName);

    if (keys.empty() || keysLC.count("type") == 1)
    {
        info["Type"] = ToString(variable.m_Type);
    }

    if (keys.empty() || keysLC.count("availablestepscount") == 1)
    {
        info["AvailableStepsCount"] =
            helper::ValueToString(variable.m_AvailableStepsCount);
    }

    if (keys.empty() || keysLC.count("shape") == 1)
    {
        // expensive function
        info["Shape"] = helper::VectorToCSV(variable.Shape());
    }

    if (keys.empty() || keysLC.count("singlevalue") == 1)
    {
        const std::string isSingleValue =
            variable.m_SingleValue ? "true" : "false";
        info["SingleValue"] = isSingleValue;
    }

    if (keys.empty() || (keysLC.count("min") == 1 && keysLC.count("max") == 1))
    {
        const auto pairMinMax = variable.MinMax();
        info["Min"] = helper::ValueToString(pairMinMax.first);
        info["Max"] = helper::ValueToString(pairMinMax.second);
    }
    else if (keysLC.count("min") == 1)
    {
        info["Min"] = helper::ValueToString(variable.Min());
    }
    else if (keysLC.count("max") == 1)
    {
        info["Max"] = helper::ValueToString(variable.Min());
    }
    return info;
}

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_IO_TCC_ */
