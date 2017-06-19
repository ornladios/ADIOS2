/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosXML.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosXML.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <sstream>
#include <stdexcept> //std::invalid_argument
/// \endcond

#include "adios2/ADIOSTypes.h"
#include "adios2/helper/adiosString.h"

namespace adios
{

void RemoveCommentsXML(std::string &currentContent) noexcept
{
    std::string::size_type startComment(currentContent.find("<!--"));

    while (startComment != currentContent.npos)
    {
        std::string::size_type endComment(currentContent.find("-->"));
        currentContent.erase(startComment, endComment - startComment + 3);
        startComment = currentContent.find("<!--");
    }
}

TagXML GetTagXML(const std::string tagName, const std::string &content,
                 std::string::size_type &position)
{
    auto lf_SetPositions =
        [](const std::string input, const std::string &content,
           std::string::size_type &position) -> std::string::size_type {

        const std::string::size_type inputPosition =
            GetStringPositionXML(input, content, position);

        if (inputPosition != std::string::npos)
        {
            position = inputPosition + input.size();
        }
        return inputPosition;
    };

    TagXML tagXML;

    std::string name(tagName);
    if (name.back() == ' ')
    {
        name.pop_back();
    }

    auto openingStart = lf_SetPositions("<" + name, content, position);
    auto openingEnd = lf_SetPositions(">", content, position);

    if (openingStart == std::string::npos || openingEnd == std::string::npos)
    {
        tagXML.IsFull = false;
        return tagXML;
    }

    tagXML.Header = content.substr(openingStart, openingEnd + 1 - openingStart);

    auto closingStart =
        GetStringPositionXML("</" + name + ">", content, position);

    if (closingStart == std::string::npos)
    {
        throw std::invalid_argument(
            "ERROR: could not find closing tag </" + name +
            "> in XML config file, in call to ADIOS constructor\n");
    }
    tagXML.IsFull = true;
    tagXML.Elements =
        content.substr(openingEnd + 1, closingStart - (openingEnd + 1));

    //    std::cout << "START..." << tagXML.Header << "...";
    //    std::cout << tagXML.Elements << "...END\n";

    return tagXML;
}

std::string::size_type
GetStringPositionXML(const std::string input, const std::string &content,
                     const std::string::size_type &startPosition) noexcept
{
    std::string::size_type foundPosition(content.find(input, startPosition));
    if (foundPosition == content.npos)
    {
        return foundPosition;
    }

    // check if it is not inside " " or ' '
    std::string::size_type currentPosition(startPosition);

    while (foundPosition != content.npos)
    {
        const std::string::size_type singleQuotePosition(
            content.find('\'', currentPosition));
        const std::string::size_type doubleQuotePosition(
            content.find('\"', currentPosition));

        if ((singleQuotePosition == content.npos &&
             doubleQuotePosition == content.npos) ||
            (singleQuotePosition == content.npos &&
             foundPosition < doubleQuotePosition) ||
            (doubleQuotePosition == content.npos &&
             foundPosition < singleQuotePosition) ||
            (foundPosition < singleQuotePosition &&
             foundPosition < doubleQuotePosition))
        {
            break;
        }
        // find the closing corresponding quote
        std::string::size_type closingQuotePosition;

        if (singleQuotePosition != content.npos &&
            doubleQuotePosition == content.npos)
        {
            currentPosition = singleQuotePosition;
            closingQuotePosition = content.find('\'', currentPosition + 1);
        }
        else if (singleQuotePosition == content.npos &&
                 doubleQuotePosition != content.npos)
        {
            currentPosition = doubleQuotePosition;
            closingQuotePosition = content.find('\"', currentPosition + 1);
        }
        else
        {
            if (singleQuotePosition < doubleQuotePosition)
            {
                currentPosition = singleQuotePosition;
                closingQuotePosition = content.find('\'', currentPosition + 1);
            }
            else
            {
                currentPosition = doubleQuotePosition;
                closingQuotePosition = content.find('\"', currentPosition + 1);
            }
        }
        // if can't find closing it's open until the end
        if (closingQuotePosition == content.npos)
        {
            currentPosition == content.npos;
            break;
        }

        currentPosition = closingQuotePosition + 1;

        if (closingQuotePosition < foundPosition)
        {
            continue;
        }
        else
        {
            // if this point is reached it means it's a value inside " " or ' ',
            // iterate
            foundPosition = content.find(input, currentPosition);
            currentPosition = foundPosition;
        }
    }

    return foundPosition;
}

Params GetTagAttributesXML(const std::string &tagHeader)
{
    auto lf_GetQuotedValue = [](const char quote,
                                const std::string::size_type &quotePosition,
                                std::string &currentTag) -> std::string {

        currentTag = currentTag.substr(quotePosition + 1);
        auto nextQuotePosition = currentTag.find(quote);

        if (nextQuotePosition == currentTag.npos)
        {
            throw std::invalid_argument(
                "ERROR: Invalid attribute in..." + currentTag +
                "...check XML file, in call to ADIOS constructor\n");
        }

        const std::string value(currentTag.substr(0, nextQuotePosition));
        currentTag = currentTag.substr(nextQuotePosition + 1);
        return value;
    };

    auto lf_GetAttributes = [&](const std::string &tag) -> Params {
        Params attributes;
        std::string currentTag(tag.substr(tag.find_first_of(" \t\n")));

        while (currentTag.find('=') != currentTag.npos) // equalPosition
        {
            currentTag =
                currentTag.substr(currentTag.find_first_not_of(" \t\n"));
            auto equalPosition = currentTag.find('=');
            if (currentTag.size() <= equalPosition + 1)
            {
                throw std::invalid_argument(
                    "ERROR: tag " + tag +
                    " is incomplete, check XML config file, "
                    "in call to ADIOS constructor\n");
            }

            const std::string key(currentTag.substr(0, equalPosition));
            std::string value;

            const char quote = currentTag[equalPosition + 1];
            if (quote == '\'' || quote == '"')
            {
                value = lf_GetQuotedValue(quote, equalPosition + 1, currentTag);
            }
            else
            {
                // throw exception here?
            }

            attributes.emplace(key, value);
        }
        return attributes;
    };

    // BODY of function starts here
    Params attributes;
    // eliminate < >
    std::string openingTag = tagHeader.substr(1, tagHeader.size() - 2);

    if (tagHeader.back() == '/') // last char is / --> "XML empty tag"
    {
        // attributes = lf_GetAttributes(openingTag);
        // throw exception here, ADIOS2 doesn't allow XML empty tags
    }
    else if (tagHeader[0] == '/') // first char is / ---> closing tag
    {
        attributes = lf_GetAttributes(openingTag);
        if (attributes.size() > 0)
        {
            throw std::invalid_argument(
                "ERROR: closing tag " + tagHeader +
                " can't have attributes, in call to ADIOS constructor\n");
        }
    }
    else // opening tag
    {
        attributes = lf_GetAttributes(openingTag);
    }
    return attributes;
}

void InitXML(const std::string configXML, const MPI_Comm mpiComm,
             const bool debugMode,
             std::vector<std::shared_ptr<Transform>> &transforms,
             std::map<std::string, IO> &ios)
{
    // independent IO
    std::string fileContents(FileToString(configXML));
    RemoveCommentsXML(fileContents);

    // adios-config
    std::string::size_type position(0);
    const TagXML adiosConfigXML(
        GetTagXML("adios-config", fileContents, position));

    // process transforms, not yet implemented

    while (position != std::string::npos)
    {
        const TagXML transformXML(
            GetTagXML("transform ", adiosConfigXML.Elements, position));

        if (transformXML.Header.empty())
        {
            break;
        }
        // InitTransform(transformTag, debugMode, transforms);
    }

    position = 0;
    // process IOs
    while (position != std::string::npos)
    {
        // io
        const TagXML ioXML(GetTagXML("io ", adiosConfigXML.Elements, position));

        if (ioXML.Header.empty()) // no more groups to find
        {
            break;
        }
        InitIOXML(ioXML, mpiComm, debugMode, transforms, ios);
    }
}

void InitIOXML(const TagXML &ioXML, const MPI_Comm mpiComm,
               const bool debugMode,
               std::vector<std::shared_ptr<Transform>> &transforms,
               std::map<std::string, IO> &ios)
{
    const Params ioAttributes(GetTagAttributesXML(ioXML.Header));

    std::string ioName;
    for (const auto &ioAttribute : ioAttributes)
    {
        if (ioAttribute.first == "name")
        {
            ioName = ioAttribute.second;
        }
    }

    if (debugMode)
    {
        if (ioName.empty())
        {
            throw std::invalid_argument(
                "ERROR: io name=\"value\" attribute not found in opening XML "
                "tag " +
                ioXML.Header +
                ", check XML config file, in call to ADIOS constructor\n");
        }

        if (ios.count(ioName) == 1) // io exists
        {
            throw std::invalid_argument("ERROR: io name " + ioName +
                                        " must be unique in XML config file, "
                                        "in call to ADIOS constructor\n");
        }
    }

    // emplace io with inConfigFile argument as true
    auto itIO = ios.emplace(ioName, IO(ioName, mpiComm, true, debugMode));

    // process engine
    std::string::size_type position(0);

    TagXML engineXML(GetTagXML("engine ", ioXML.Elements, position));
    if (!engineXML.Header.empty()) // found first one
    {
        InitEngineXML(engineXML, debugMode, itIO.first->second);
    }

    if (debugMode)
    {
        // try finding a 2nd one from current position
        TagXML engineXML(GetTagXML("engine ", ioXML.Elements, position));
        if (!engineXML.Header.empty()) // found first one
        {
            throw std::invalid_argument(
                "ERROR: more than one engine found in <io name=" + ioName +
                "...>, only one per io tag is allowed in XML "
                "config file, in call to "
                "ADIOS constructor\n");
        }
    }

    position = 0;
    // process transports
    while (position != std::string::npos)
    {
        TagXML transportXML(GetTagXML("transport", ioXML.Elements, position));

        if (transportXML.Header.empty()) // no more groups to find
        {
            break;
        }
        InitTransportXML(transportXML, debugMode, itIO.first->second);
    }
}

void InitEngineXML(const TagXML &engineXML, const bool debugMode, IO &io)
{
    const Params attributes = GetTagAttributesXML(engineXML.Header);

    std::string type;
    for (const auto &attribute : attributes)
    {
        if (attribute.first == "type")
        {
            type = attribute.second;
            break;
        }
    }

    if (!type.empty())
    {
        io.SetEngine(type);
    }

    io.SetParameters(ParseParamsXML(engineXML.Elements, debugMode));
}

void InitTransportXML(const TagXML &transportXML, const bool debugMode, IO &io)
{
    const Params attributes = GetTagAttributesXML(transportXML.Header);

    std::string type;
    for (const auto &attribute : attributes)
    {
        if (attribute.first == "type")
        {
            type = attribute.second;
            break;
        }
    }

    if (type.empty())
    {
        throw std::invalid_argument(
            "ERROR: missing transport type in " + transportXML.Header +
            ", in XML config file, in call to ADIOS constructor\n");
    }

    io.AddTransport(type, ParseParamsXML(transportXML.Elements, debugMode));
}

Params ParseParamsXML(const std::string &tagElements, const bool debugMode)
{
    auto start = tagElements.find_first_not_of(" \t\n");
    auto end = tagElements.find_last_not_of(" \t\n");

    std::string parametersString(tagElements.substr(start, end - start + 1));
    if (debugMode)
    {
        if (parametersString.back() != ';')
        {
            throw std::invalid_argument(
                "ERROR: parameters in config XML file must end with a ; " +
                tagElements + ", in call to ADIOS constructor\n");
        }
    }

    std::istringstream parametersSS(parametersString);
    std::string pair;

    Params parameters;

    while (std::getline(parametersSS, pair, ';'))
    {
        pair = pair.substr(pair.find_first_not_of(" \t\n"));
        auto equalPosition = pair.find("=");

        if (debugMode)
        {
            if (equalPosition == std::string::npos ||
                equalPosition == pair.size())
            {
                throw std::invalid_argument("ERROR: wrong parameter " + pair +
                                            " format is "
                                            "key=value in XML config file, in "
                                            "call to ADIOS constructor\n");
            }
        }

        const std::string key(pair.substr(0, equalPosition));
        const std::string value(pair.substr(equalPosition + 1));
        parameters.emplace(key, value);
    }
    return parameters;
}

} // end namespace adios
