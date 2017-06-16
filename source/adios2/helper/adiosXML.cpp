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
#include "adios2/helper/adiosSystem.h"

namespace adios
{

std::string GetSubString(const std::string initialTag,
                         const std::string finalTag, const std::string &content,
                         std::string::size_type &currentPosition)
{
    auto lf_Wipe = [](std::string &subString,
                      std::string::size_type &currentPosition) {
        subString.clear();
        currentPosition = std::string::npos;
    };

    auto lf_SetPositions =
        [](const char quote, const std::string::size_type quotePosition,
           const std::string &content, std::string::size_type &currentPosition,
           std::string::size_type &closingQuotePosition) {
            currentPosition = quotePosition;
            closingQuotePosition = content.find(quote, currentPosition + 1);
        };

    // BODY OF FUNCTION STARTS HERE
    std::string subString;

    std::string::size_type start(content.find(initialTag, currentPosition));
    if (start == content.npos)
    {
        lf_Wipe(subString, currentPosition);
        return subString;
    }
    currentPosition = start;

    std::string::size_type end(content.find(finalTag, currentPosition));
    if (end == content.npos)
    {
        lf_Wipe(subString, currentPosition);
        return subString;
    }

    // make sure the finalTag is not a value surrounded by " " or ' ',
    // if so find next
    bool isValue = true;

    while (isValue)
    {
        std::string::size_type singleQuotePosition =
            content.find('\'', currentPosition);
        std::string::size_type doubleQuotePosition =
            content.find('\"', currentPosition);

        if ((singleQuotePosition == content.npos &&
             doubleQuotePosition == content.npos) ||
            (singleQuotePosition == content.npos &&
             end < doubleQuotePosition) ||
            (doubleQuotePosition == content.npos &&
             end < singleQuotePosition) ||
            (end < singleQuotePosition && end < doubleQuotePosition))
        {
            break;
        }

        // find the closing corresponding quote
        std::string::size_type closingQuotePosition;

        if (singleQuotePosition == content.npos)
        { // no ' anywhere
            lf_SetPositions('\"', doubleQuotePosition, content, currentPosition,
                            closingQuotePosition);
        }
        else if (doubleQuotePosition == content.npos)
        { // no " anywhere
            lf_SetPositions('\'', singleQuotePosition, content, currentPosition,
                            closingQuotePosition);
        }
        else
        {
            if (singleQuotePosition < doubleQuotePosition)
            {
                lf_SetPositions('\'', singleQuotePosition, content,
                                currentPosition, closingQuotePosition);
            }
            else
            { // find the closing "
                lf_SetPositions('\"', doubleQuotePosition, content,
                                currentPosition, closingQuotePosition);
            }
        }

        // if can't find closing it's open until the end
        if (closingQuotePosition == content.npos)
        {
            lf_Wipe(subString, currentPosition);
            return subString;
        }

        currentPosition = closingQuotePosition + 1;

        if (closingQuotePosition < end)
        {
            continue;
        }

        // if this point is reached it means it's a value inside " " or ' ',
        // move to the next end
        end = content.find(finalTag, currentPosition);
    }

    currentPosition = end;
    subString = content.substr(start, end - start + finalTag.size());
    return subString;
}

std::string GetOpeningTag(const std::string tagName,
                          const std::string &tagContents,
                          std::string::size_type &position,
                          const bool debugMode)
{
    std::string openingTag(
        GetSubString("<" + tagName + " ", ">", tagContents, position));

    const std::string::size_type elementsStartPosition(position);

    if (debugMode)
    {
        if (openingTag.size() < 2)
        {
            throw std::invalid_argument("ERROR: wrong XML tag <" + tagName +
                                        ", in call to ADIOS constructor\n");
        }
    }

    // eliminate < >
    openingTag = openingTag.substr(1, openingTag.size() - 2);
    return openingTag;
}

Params GetTagAttributes(const std::string &fileContent, const std::string &tag)
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

        currentTag = currentTag.substr(nextQuotePosition + 1);
        return currentTag.substr(0, nextQuotePosition);
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

            attributes.emplace(key, value);
        }
        return attributes;
    };

    // BODY of function starts here
    Params attributes;

    if (tag.back() == '/') // last char is / --> "XML empty tag"
    {
        attributes = lf_GetAttributes(tag);
    }
    else if (tag[0] == '/') // first char is / ---> closing tag
    {
        attributes = lf_GetAttributes(tag);
        if (attributes.size() > 0)
        {
            throw std::invalid_argument(
                "ERROR: closing tag " + tag +
                " can't have attributes, in call to ADIOS constructor\n");
        }
    }
    else // opening tag
    {
        const std::string tagName(tag.substr(0, tag.find_first_of(" \t\n\r")));
        // look for closing tagName
        const std::string closingTagName("</" + tagName + ">");

        if (fileContent.find(closingTagName) == fileContent.npos)
        {
            throw std::invalid_argument(
                "ERROR: closing tag missing for " + closingTagName +
                " check XML config file, in call to ADIOS constructor\n");
        }

        attributes = lf_GetAttributes(tag);
    }
    return attributes;
}

void RemoveXMLComments(std::string &currentContent) noexcept
{
    std::string::size_type startComment(currentContent.find("<!--"));

    while (startComment != currentContent.npos)
    {
        std::string::size_type endComment(currentContent.find("-->"));
        currentContent.erase(startComment, endComment - startComment + 3);
        startComment = currentContent.find("<!--");
    }
}

void InitXML(const std::string configXML, const MPI_Comm mpiComm,
             const bool debugMode,
             std::vector<std::shared_ptr<Transform>> &transforms,
             std::map<std::string, IO> &ios)
{
    // if using collective IO only?
    std::string fileContents = BroadcastFileContents(configXML, mpiComm);

    // adios-config
    std::string::size_type currentPosition(0);
    std::string currentContent(GetSubString("<adios-config ", "</adios-config>",
                                            fileContents, currentPosition));
    RemoveXMLComments(currentContent);

    // process transforms, not yet implemented
    while (currentPosition != std::string::npos)
    {
        std::string transformTag(GetSubString("<transform ", "</transform>",
                                              currentContent, currentPosition));

        if (transformTag.empty()) // no more transforms
        {
            break;
        }
        // InitTransform(transformTag, debugMode, transforms);
    }

    currentPosition = 0;
    // process IOs
    while (currentPosition != std::string::npos)
    {
        // io
        std::string ioTag(
            GetSubString("<io ", "</io>", currentContent, currentPosition));

        if (ioTag.empty()) // no more groups to find
        {
            break;
        }
        InitIOXML(ioTag, mpiComm, debugMode, transforms, ios);
    }
}

void InitIOXML(const std::string &ioTag, const MPI_Comm mpiComm,
               const bool debugMode,
               std::vector<std::shared_ptr<Transform>> &transforms,
               std::map<std::string, IO> &ios)
{
    std::string::size_type position(0);
    const std::string openingTag(
        GetOpeningTag("io", ioTag, position, debugMode));
    const std::string::size_type elementsStart(position);

    Params ioAttributes = GetTagAttributes(ioTag, openingTag);

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
                openingTag +
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
    std::string engineTag(
        GetSubString("<engine ", "</engine>", ioTag, position));

    if (!engineTag.empty()) // no more groups to find
    {
        InitEngineXML(engineTag, debugMode, itIO.first->second);
    }

    if (debugMode)
    {
        // try finding a 2nd one
        std::string engineTag(
            GetSubString("<engine ", "</engine>", ioTag, position));
        if (!engineTag.empty())
        {
            throw std::invalid_argument("ERROR: two engine tags found in IO " +
                                        ioName + ", only one is allowed in XML "
                                                 "config file, in call to "
                                                 "ADIOS constructor\n");
        }
    }

    position = elementsStart;
    // process transports
    while (position != std::string::npos)
    {
        std::string transportTag(
            GetSubString("<transport ", "</transport>", ioTag, position));

        if (transportTag.empty()) // no more groups to find
        {
            break;
        }
    }
}

void InitEngineXML(const std::string &engineTag, const bool debugMode, IO &io)
{
    std::string::size_type position(0);
    const std::string openingTag(
        GetOpeningTag("engine", engineTag, position, debugMode));
    const std::string::size_type elementsStartPosition(position);
    Params attributes = GetTagAttributes(engineTag, openingTag);

    std::string type;
    for (const auto &attribute : attributes)
    {
        if (attribute.first == "type")
        {
            type = attribute.second;
        }
    }

    io.SetEngine(type);
    io.SetParameters(
        ParseParamsXML(engineTag, elementsStartPosition, debugMode));
}

void InitTransportXML(const std::string &transportTag, const bool debugMode,
                      IO &io)
{
    std::string::size_type position(0);
    const std::string openingTag(
        GetOpeningTag("transport", transportTag, position, debugMode));
    const std::string::size_type elementsStartPosition(position);
    Params attributes = GetTagAttributes(transportTag, openingTag);

    std::string type;
    for (const auto &attribute : attributes)
    {
        if (attribute.first == "type")
        {
            type = attribute.second;
        }
    }

    io.AddTransport(
        type, ParseParamsXML(transportTag, elementsStartPosition, debugMode));
}

Params ParseParamsXML(const std::string &tag,
                      const std::string::size_type elementsStartPosition,
                      const bool debugMode)
{
    std::istringstream parametersSS(tag.substr(elementsStartPosition));
    std::string pair;

    Params parameters;

    while (std::getline(parametersSS, pair, ';'))
    {
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
