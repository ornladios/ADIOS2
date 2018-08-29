/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS.h"

#include <algorithm> // std::transform
#include <ios>       //std::ios_base::failure

#include "adios2/ADIOSMPI.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //InquireKey, BroadcastFile

// OPERATORS

// compress
#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZip2.h"
#endif

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZfp.h"
#endif

#ifdef ADIOS2_HAVE_SZ
#include "adios2/operator/compress/CompressSZ.h"
#endif

// callbacks
#include "adios2/operator/callback/Signature1.h"
#include "adios2/operator/callback/Signature2.h"

namespace adios2
{
namespace core
{

ADIOS::ADIOS(const std::string configFile, MPI_Comm mpiComm,
             const bool debugMode, const std::string hostLanguage)
: m_MPIComm(mpiComm), m_ConfigFile(configFile), m_DebugMode(debugMode),
  m_HostLanguage(hostLanguage)
{
    if (m_DebugMode)
    {
        CheckMPI();
    }
    if (!configFile.empty())
    {
        if (configFile.substr(configFile.size() - 3) == "xml")
        {
            XMLInit(configFile);
        }
        // TODO expand for other formats
    }
}

ADIOS::ADIOS(const std::string configFile, const bool debugMode,
             const std::string hostLanguage)
: ADIOS(configFile, MPI_COMM_SELF, debugMode, hostLanguage)
{
}

ADIOS::ADIOS(MPI_Comm mpiComm, const bool debugMode,
             const std::string hostLanguage)
: ADIOS("", mpiComm, debugMode, hostLanguage)
{
}

ADIOS::ADIOS(const bool debugMode, const std::string hostLanguage)
: ADIOS("", MPI_COMM_SELF, debugMode, hostLanguage)
{
}

IO &ADIOS::DeclareIO(const std::string name)
{
    auto itIO = m_IOs.find(name);

    if (itIO != m_IOs.end())
    {
        IO &io = itIO->second;

        if (!io.IsDeclared()) // exists from config xml
        {
            io.SetDeclared();
            return io;
        }
        else
        {
            if (m_DebugMode)
            {
                throw std::invalid_argument(
                    "ERROR: IO with name " + name +
                    " previously declared with DeclareIO, name must be unique,"
                    " in call to DeclareIO\n");
            }
        }
    }

    auto ioPair = m_IOs.emplace(
        name, IO(*this, name, m_MPIComm, false, m_HostLanguage, m_DebugMode));
    IO &io = ioPair.first->second;
    io.SetDeclared();
    return io;
}

IO &ADIOS::AtIO(const std::string name)
{
    auto itIO = m_IOs.find(name);

    if (itIO == m_IOs.end())
    {
        throw std::invalid_argument("ERROR: IO with name " + name +
                                    " was not declared, did you previously "
                                    "call DeclareIO?, in call to AtIO\n");
    }
    else
    {
        if (!itIO->second.IsDeclared())
        {
            throw std::invalid_argument("ERROR: IO with name " + name +
                                        " was not declared, did you previously "
                                        "call DeclareIO ?, in call to AtIO\n");
        }
    }

    return itIO->second;
}

void ADIOS::FlushAll()
{
    for (auto &ioPair : m_IOs)
    {
        ioPair.second.FlushAll();
    }
}

Operator &ADIOS::DefineOperator(const std::string name, const std::string type,
                                const Params &parameters)
{
    std::shared_ptr<Operator> operatorPtr;

    CheckOperator(name);
    std::string typeLowerCase(type);
    std::transform(typeLowerCase.begin(), typeLowerCase.end(),
                   typeLowerCase.begin(), ::tolower);

    if (typeLowerCase == "bzip2")
    {
#ifdef ADIOS2_HAVE_BZIP2
        auto itPair = m_Operators.emplace(
            name,
            std::make_shared<compress::CompressBZip2>(parameters, m_DebugMode));
        operatorPtr = itPair.first->second;
#else
        throw std::invalid_argument(
            "ERROR: this version of ADIOS2 didn't compile with the "
            "bzip2 library, in call to DefineOperator\n");
#endif
    }
    else if (typeLowerCase == "zfp")
    {
#ifdef ADIOS2_HAVE_ZFP
        auto itPair = m_Operators.emplace(
            name,
            std::make_shared<compress::CompressZfp>(parameters, m_DebugMode));
        operatorPtr = itPair.first->second;
#else
        throw std::invalid_argument(
            "ERROR: this version of ADIOS2 didn't compile with the "
            "zfp library (minimum v1.5), in call to DefineOperator\n");
#endif
    }
    else if (typeLowerCase == "sz")
    {
#ifdef ADIOS2_HAVE_SZ
        auto itPair = m_Operators.emplace(
            name,
            std::make_shared<compress::CompressSZ>(parameters, m_DebugMode));
        operatorPtr = itPair.first->second;
#else
        throw std::invalid_argument(
            "ERROR: this version of ADIOS2 didn't compile with the "
            "SZ library (minimum v2.0.2.0), in call to DefineOperator\n");
#endif
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: Operator " + name + " of type " + type +
                " is not supported by ADIOS2, in call to DefineOperator\n");
        }
    }

    if (m_DebugMode && !operatorPtr)
    {
        throw std::invalid_argument(
            "ERROR: Operator " + name + " of type " + type +
            " couldn't be defined, in call to DefineOperator\n");
    }

    return *operatorPtr.get();
}

Operator *ADIOS::InquireOperator(const std::string name) noexcept
{
    return helper::InquireKey(name, m_Operators)->get();
}

#define declare_type(T)                                                        \
    Operator &ADIOS::DefineCallBack(                                           \
        const std::string name,                                                \
        const std::function<void(const T *, const std::string &,               \
                                 const std::string &, const std::string &,     \
                                 const size_t, const Dims &, const Dims &,     \
                                 const Dims &)> &function,                     \
        const Params &parameters)                                              \
    {                                                                          \
        CheckOperator(name);                                                   \
        std::shared_ptr<Operator> callbackOperator =                           \
            std::make_shared<callback::Signature1>(function, parameters,       \
                                                   m_DebugMode);               \
                                                                               \
        auto itPair = m_Operators.emplace(name, std::move(callbackOperator));  \
        return *itPair.first->second;                                          \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

Operator &ADIOS::DefineCallBack(
    const std::string name,
    const std::function<void(void *, const std::string &, const std::string &,
                             const std::string &, const size_t, const Dims &,
                             const Dims &, const Dims &)> &function,
    const Params &parameters)
{
    CheckOperator(name);
    std::shared_ptr<Operator> callbackOperator =
        std::make_shared<callback::Signature2>(function, parameters,
                                               m_DebugMode);

    auto itPair = m_Operators.emplace(name, std::move(callbackOperator));
    return *itPair.first->second;
}

// PRIVATE FUNCTIONS
void ADIOS::CheckMPI() const
{
    if (m_MPIComm == MPI_COMM_NULL)
    {
        throw std::ios_base::failure("ERROR: MPI communicator is MPI_COMM_NULL,"
                                     " in call to ADIOS constructor\n");
    }
}

void ADIOS::CheckOperator(const std::string name) const
{
    if (m_DebugMode)
    {
        if (m_Operators.count(name) == 1)
        {
            throw std::invalid_argument(
                "ERROR: Operator with name " + name +
                ", is already defined in either config file "
                "or with call to DefineOperator, name must "
                "be unique, in call to DefineOperator\n");
        }
    }
}

// requires pugi
void ADIOS::XMLInit(const std::string configXML)
{
    const std::string hint("for config file " + configXML +
                           " in call to ADIOS constructor");

    auto lf_FileContents = [&](const std::string configXML) -> std::string {

        const std::string fileContents(helper::BroadcastFile(
            configXML, m_MPIComm,
            "when parsing configXML file, in call to the ADIOS constructor"));

        if (m_DebugMode)
        {
            if (fileContents.empty())
            {
                throw std::invalid_argument(
                    "ERROR: config xml file is empty, " + hint + "\n");
            }
        }
        return fileContents;
    };

    auto lf_GetParametersXML = [&](const pugi::xml_node &node) -> Params {

        const std::string errorMessage("in node " + std::string(node.value()) +
                                       ", " + hint);
        Params parameters;

        for (const pugi::xml_node paramNode : node.children("parameter"))
        {
            const pugi::xml_attribute key = helper::XMLAttribute(
                "key", paramNode, m_DebugMode, errorMessage);

            const pugi::xml_attribute value = helper::XMLAttribute(
                "value", paramNode, m_DebugMode, errorMessage);

            parameters.emplace(key.value(), value.value());
        }
        return parameters;
    };

    auto lf_OperatorXML = [&](const pugi::xml_node &operatorNode) {

        const pugi::xml_attribute name =
            helper::XMLAttribute("name", operatorNode, m_DebugMode, hint);

        const pugi::xml_attribute type =
            helper::XMLAttribute("type", operatorNode, m_DebugMode, hint);

        const Params parameters = lf_GetParametersXML(operatorNode);

        DefineOperator(name.value(), type.value(), parameters);
    };

    // node is the variable node
    auto lf_IOVariableXML = [&](const pugi::xml_node node,
                                core::IO &currentIO) {

        const std::string variableName = std::string(
            helper::XMLAttribute("name", node, m_DebugMode, hint).value());

        for (const pugi::xml_node operation : node.children("operation"))
        {
            const pugi::xml_attribute opName = helper::XMLAttribute(
                "operator", operation, m_DebugMode, hint, false);

            const pugi::xml_attribute opType = helper::XMLAttribute(
                "type", operation, m_DebugMode, hint, false);

            if (m_DebugMode)
            {
                if (opName && opType)
                {
                    throw std::invalid_argument(
                        "ERROR: operator (" + std::string(opName.value()) +
                        ") and type (" + std::string(opType.value()) +
                        ") attributes can't coexist in <operation> element "
                        "inside <variable name=\"" +
                        variableName + "\"> element, " + hint + "\n");
                }

                if (!opName && !opType)
                {
                    throw std::invalid_argument(
                        "ERROR: <operation> element "
                        "inside <variable name=\"" +
                        variableName +
                        "\"> element requires either operator "
                        "(existing) or type (supported) attribute, " +
                        hint + "\n");
                }
            }

            core::Operator *op = nullptr;

            if (opName)
            {
                auto itOperator = m_Operators.find(std::string(opName.value()));
                if (m_DebugMode)
                {
                    if (itOperator == m_Operators.end())
                    {
                        throw std::invalid_argument(
                            "ERROR: operator " + std::string(opName.value()) +
                            " not previously defined, from variable " +
                            variableName + " inside io " +
                            std::string(currentIO.m_Name) + ", " + hint + "\n");
                    }
                }
                op = itOperator->second.get();
            }

            if (opType)
            {
                const std::string operatorType = std::string(opType.value());
                const std::string operatorName =
                    "__" + currentIO.m_Name + "_" + operatorType;
                auto itOperator = m_Operators.find(operatorName);

                if (itOperator == m_Operators.end())
                {
                    op = &DefineOperator(operatorName, operatorType);
                }
                else
                {
                    op = itOperator->second.get();
                }
            }
            const Params parameters = lf_GetParametersXML(operation);
            currentIO.m_VarOpsPlaceholder[variableName].push_back(
                core::IO::Operation{op, parameters, Params()});
        }
    };

    auto lf_IOXML = [&](const pugi::xml_node &io) {

        const pugi::xml_attribute ioName =
            helper::XMLAttribute("name", io, m_DebugMode, hint);

        // Build the IO object
        auto itCurrentIO = m_IOs.emplace(
            ioName.value(), core::IO(*this, ioName.value(), m_MPIComm, true,
                                     m_HostLanguage, m_DebugMode));
        core::IO &currentIO = itCurrentIO.first->second;

        // must be unique per io
        const pugi::xml_node engine =
            helper::XMLNode("engine", io, m_DebugMode, hint, false, true);

        if (engine)
        {
            const pugi::xml_attribute type =
                helper::XMLAttribute("type", engine, m_DebugMode, hint);
            currentIO.SetEngine(type.value());

            const Params parameters = lf_GetParametersXML(engine);
            currentIO.SetParameters(parameters);
        }

        for (const pugi::xml_node variable : io.children("variable"))
        {
            lf_IOVariableXML(variable, currentIO);
        }
    };

    // BODY OF FUNCTION
    const std::string fileContents = lf_FileContents(configXML);
    const pugi::xml_document document =
        helper::XMLDocument(fileContents, m_DebugMode, hint);

    // must be unique
    const pugi::xml_node config =
        helper::XMLNode("adios-config", document, m_DebugMode, hint, true);

    for (const pugi::xml_node op : config.children("operator"))
    {
        lf_OperatorXML(op);
    }

    for (const pugi::xml_node io : config.children("io"))
    {
        lf_IOXML(io);
    }
}

} // end namespace core
} // end namespace adios2
