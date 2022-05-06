/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11Engine.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

#include <sstream>

#include "py11types.h"

namespace adios2
{
namespace py11
{

Engine::Engine(core::Engine *engine) : m_Engine(engine) {}

Engine::operator bool() const noexcept
{
    if (m_Engine == nullptr)
    {
        return false;
    }

    return *m_Engine ? true : false;
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::BeginStep");
    return m_Engine->BeginStep(mode, timeoutSeconds);
}

StepStatus Engine::BeginStep()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::BeginStep");
    return m_Engine->BeginStep();
}

void Engine::Put(Variable variable, const pybind11::array &array,
                 const Mode launch)
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::Put numpy array");
    helper::CheckForNullptr(variable.m_VariableBase,
                            "for variable, in call to Engine::Put numpy array");

    const adios2::DataType type =
        helper::GetDataTypeFromString(variable.Type());

    if (type == adios2::DataType::Struct)
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        m_Engine->Put(                                                         \
            *dynamic_cast<core::Variable<T> *>(variable.m_VariableBase),       \
            reinterpret_cast<const T *>(array.data()), launch);                \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument("ERROR: for variable " + variable.Name() +
                                    " numpy array type is not supported or "
                                    "is not memory contiguous "
                                    ", in call to Put\n");
    }
}

void Engine::Put(Variable variable, const std::string &string)
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::Put string");
    helper::CheckForNullptr(variable.m_VariableBase,
                            "for variable, in call to Engine::Put string");

    if (helper::GetDataTypeFromString(variable.Type()) !=
        helper::GetDataType<std::string>())
    {
        throw std::invalid_argument(
            "ERROR: variable " + variable.Name() +
            " is not of string type, in call to Engine::Put");
    }

    m_Engine->Put(
        *dynamic_cast<core::Variable<std::string> *>(variable.m_VariableBase),
        string, adios2::Mode::Sync);
}

void Engine::PerformPuts()
{
    helper::CheckForNullptr(m_Engine, "in call to PerformPuts");
    m_Engine->PerformPuts();
}

void Engine::PerformDataWrite()
{
    helper::CheckForNullptr(m_Engine, "in call to PerformDataWrite");
    m_Engine->PerformDataWrite();
}

void Engine::Get(Variable variable, pybind11::array &array, const Mode launch)
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::Get a numpy array");
    helper::CheckForNullptr(
        variable.m_VariableBase,
        "for variable, in call to Engine::Get a numpy array");

    const adios2::DataType type =
        helper::GetDataTypeFromString(variable.Type());

    if (type == adios2::DataType::Struct)
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        m_Engine->Get(                                                         \
            *dynamic_cast<core::Variable<T> *>(variable.m_VariableBase),       \
            reinterpret_cast<T *>(const_cast<void *>(array.data())), launch);  \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: in variable " + variable.Name() + " of type " +
            variable.Type() +
            ", numpy array type is 1) not supported, 2) a type mismatch or"
            "3) is not memory contiguous "
            ", in call to Get\n");
    }
}

std::string Engine::Get(Variable variable, const Mode launch)
{
    std::string string;
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::Get a numpy array");
    helper::CheckForNullptr(variable.m_VariableBase,
                            "for variable, in call to Engine::Get a string");

    const adios2::DataType type =
        helper::GetDataTypeFromString(variable.Type());

    if (type == helper::GetDataType<std::string>())
    {
        m_Engine->Get(*dynamic_cast<core::Variable<std::string> *>(
                          variable.m_VariableBase),
                      string, launch);
    }
    else
    {
        throw std::invalid_argument("ERROR: variable " + variable.Name() +
                                    " of type " + variable.Type() +
                                    " is not string, in call to Engine::Get");
    }
    return string;
}
void Engine::PerformGets()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::PerformGets");
    m_Engine->PerformGets();
}

void Engine::EndStep()
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::EndStep");
    m_Engine->EndStep();
}

void Engine::Flush(const int transportIndex)
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Flush");
    m_Engine->Flush(transportIndex);
}

void Engine::Close(const int transportIndex)
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Close");
    m_Engine->Close(transportIndex);

    // erase Engine object from IO
    core::IO &io = m_Engine->GetIO();
    const std::string name = m_Engine->m_Name;
    io.RemoveEngine(name);
    m_Engine = nullptr;
}

size_t Engine::CurrentStep() const
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::CurrentStep");
    return m_Engine->CurrentStep();
}

std::string Engine::Name() const
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Name");
    return m_Engine->m_Name;
}

std::string Engine::Type() const
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Type");
    return m_Engine->m_EngineType;
}

size_t Engine::Steps() const
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Steps");
    return m_Engine->Steps();
}

void Engine::LockWriterDefinitions() const
{
    helper::CheckForNullptr(m_Engine,
                            "in call to Engine::LockWriterDefinitions");
    m_Engine->LockWriterDefinitions();
}

void Engine::LockReaderSelections() const
{
    helper::CheckForNullptr(m_Engine,
                            "in call to Engine::LockReaderSelections");
    m_Engine->LockReaderSelections();
}

std::vector<std::map<std::string, std::string>>
Engine::BlocksInfo(std::string &var_name, const size_t step) const
{
    std::vector<std::map<std::string, std::string>> rv;

    // Grab the specified variable object and get its type string
    adios2::DataType var_type = m_Engine->GetIO().InquireVariableType(var_name);

    // Use the macro incantation to call the right instantiation of
    // core::BlocksInfo<>() Note that we are flatting the Dims type items, and
    // returning everything as a dictionary of strings.
    if (false)
    {
    }
#define GET_BLOCKS_INFO(T)                                                     \
    else if (var_type == helper::GetDataType<T>())                             \
    {                                                                          \
        auto variable = m_Engine->GetIO().InquireVariable<T>(var_name);        \
        auto infoVec = m_Engine->BlocksInfo<T>(*variable, step);               \
        for (auto &info : infoVec)                                             \
        {                                                                      \
            std::map<std::string, std::string> info_map;                       \
            std::stringstream start_ss;                                        \
            for (size_t i = 0; i < info.Start.size(); ++i)                     \
            {                                                                  \
                if (i != 0)                                                    \
                    start_ss << ",";                                           \
                start_ss << info.Start[i];                                     \
            }                                                                  \
            info_map["Start"] = start_ss.str();                                \
            std::stringstream count_ss;                                        \
            for (size_t i = 0; i < info.Count.size(); ++i)                     \
            {                                                                  \
                if (i != 0)                                                    \
                    count_ss << ",";                                           \
                count_ss << info.Count[i];                                     \
            }                                                                  \
            info_map["Count"] = count_ss.str();                                \
            info_map["WriterID"] = std::to_string(info.WriterID);              \
            info_map["BlockID"] = std::to_string(info.BlockID);                \
            info_map["IsValue"] = info.IsValue ? "True" : "False";             \
            std::ostringstream osMax, osMin;                                   \
            osMax << info.Max;                                                 \
            osMin << info.Min;                                                 \
            info_map["Max"] = osMax.str();                                     \
            info_map["Min"] = osMin.str();                                     \
            info_map["IsReverseDims"] = info.IsReverseDims ? "True" : "False"; \
            rv.push_back(info_map);                                            \
        }                                                                      \
    }
    ADIOS2_FOREACH_PYTHON_TYPE_1ARG(GET_BLOCKS_INFO)
#undef GET_BLOCKS_INFO
    else
    {
        throw std::invalid_argument("ERROR: variable " + var_name +
                                    " can't be defined, either type is not "
                                    "supported or is not memory "
                                    "contiguous, in call to DefineVariable\n");
    }

    return rv;
}

} // end namespace py11
} // end namespace adios2
