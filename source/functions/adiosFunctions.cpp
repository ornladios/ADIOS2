/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosFunctions.cpp
 *
 *  Created on: Oct 10, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <algorithm> //std::count
#include <cmath>     // std::ceil, std::pow, std::log
#include <cstring>   //std::memcpy
#include <fstream>
#include <ios> //std::ios_base::failure
#include <sstream>
#include <stdexcept>
#include <thread> //std::thread

#include <sys/stat.h>  //stat
#include <sys/types.h> //CreateDirectory
#include <unistd.h>    //CreateDirectory
/// \endcond

#include "core/Support.h"
#include "functions/adiosFunctions.h"

#ifdef HAVE_BZIP2
#include "transform/BZIP2.h"
#endif

namespace adios
{

void DumpFileToString(const std::string fileName, std::string &fileContent)
{
    std::ifstream fileStream(fileName);

    if (fileStream.good() == false)
    { // check file
        throw std::ios_base::failure(
            "ERROR: file " + fileName +
            " could not be opened. Check permissions or file existence\n");
    }

    std::ostringstream fileSS;
    fileSS << fileStream.rdbuf();
    fileStream.close();
    fileContent = fileSS.str(); // convert to string and check

    if (fileContent.empty())
    {
        throw std::invalid_argument("ERROR: file " + fileName + " is empty\n");
    }
}

void GetSubString(const std::string initialTag, const std::string finalTag,
                  const std::string content, std::string &subString,
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
    std::string::size_type start(content.find(initialTag, currentPosition));
    if (start == content.npos)
    {
        lf_Wipe(subString, currentPosition);
        return;
    }
    currentPosition = start;

    std::string::size_type end(content.find(finalTag, currentPosition));
    if (end == content.npos)
    {
        lf_Wipe(subString, currentPosition);
        return;
    }

    // here make sure the finalTag is not a value surrounded by " " or ' ', if
    // so
    // find next
    bool isValue = true;

    while (isValue == true)
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

        if (closingQuotePosition ==
            content.npos) // if can't find closing it's open until the end
        {
            lf_Wipe(subString, currentPosition);
            return;
        }

        currentPosition = closingQuotePosition + 1;

        if (closingQuotePosition < end)
        {
            continue;
        }

        // if this point is reached it means it's a value inside " " or ' ',
        // move to
        // the next end
        end = content.find(finalTag, currentPosition);
    }

    subString = content.substr(start, end - start + finalTag.size());
    currentPosition = end;
}

void GetQuotedValue(const char quote,
                    const std::string::size_type &quotePosition,
                    std::string &currentTag, std::string &value)
{
    currentTag = currentTag.substr(quotePosition + 1);
    auto nextQuotePosition = currentTag.find(quote);

    if (nextQuotePosition == currentTag.npos)
    {
        throw std::invalid_argument("ERROR: Invalid attribute in..." +
                                    currentTag + "...check XML file\n");
    }

    value = currentTag.substr(0, nextQuotePosition);
    currentTag = currentTag.substr(nextQuotePosition + 1);
}

void GetPairs(const std::string tag,
              std::vector<std::pair<const std::string, const std::string>>
                  &pairs) noexcept
{
    std::string currentTag(
        tag.substr(tag.find_first_of(" \t\n"))); // initialize current tag

    while (currentTag.find('=') != currentTag.npos) // equalPosition
    {
        currentTag = currentTag.substr(currentTag.find_first_not_of(" \t\n"));
        auto equalPosition = currentTag.find('=');
        const std::string field(
            currentTag.substr(0, equalPosition)); // get field
        std::string value;

        const char quote = currentTag[equalPosition + 1];
        if (quote == '\'' || quote == '"') // single quotes
        {
            GetQuotedValue(quote, equalPosition + 1, currentTag, value);
        }

        pairs.push_back(
            std::pair<const std::string, const std::string>(field, value));
    }
}

void GetPairsFromTag(
    const std::string &fileContent, const std::string tag,
    std::vector<std::pair<const std::string, const std::string>> &pairs)
{
    if (tag.back() == '/') // last char is / --> "XML empty tag"
    {
        GetPairs(tag, pairs);
    }
    else if (tag[0] == '/') // first char is / ---> closing tag
    {
    }
    else // opening tag
    {
        const std::string tagName(tag.substr(0, tag.find_first_of(" \t\n\r")));
        const std::string closingTagName("</" + tagName +
                                         ">"); // check for closing tagName

        if (fileContent.find(closingTagName) == fileContent.npos)
        {
            throw std::invalid_argument("ERROR: closing tag " + closingTagName +
                                        " missing, check XML file\n");
        }

        GetPairs(tag, pairs);
    }
}

// void SetMembers( const std::string& fileContent, const MPI_Comm mpiComm,
// const bool debugMode,
//                 std::string& hostLanguage, std::vector<
//                 std::shared_ptr<Transform> >& transforms,
//                 std::map< std::string, Group >& groups )
//{
//    //adios-config
//    std::string currentContent;
//    std::string::size_type currentPosition( 0 );
//    GetSubString( "<adios-config ", "</adios-config>", fileContent,
//    currentContent, currentPosition );
//
//    //remove comment sections
//    std::string::size_type startComment ( currentContent.find( "<!--" ) );
//
//    while( startComment != currentContent.npos )
//    {
//        std::string::size_type endComment( currentContent.find( "-->") );
//        currentContent.erase( startComment, endComment-startComment+3 );
//        startComment = currentContent.find( "<!--" );
//    }
//
//    //Tag <adios-config
//    currentPosition = 0;
//
//    std::string tag; //use for < > tags
//    GetSubString( "<adios-config", ">", currentContent, tag, currentPosition
//    );
//    tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
//
//    std::vector< std::pair<const std::string, const std::string> > pairs; //
//    pairs in tag
//    GetPairsFromTag( currentContent, tag, pairs );
//
//    for( auto& pair : pairs )
//        if( pair.first == "host-language" )
//            hostLanguage = pair.second;
//
//    if( debugMode == true )
//    {
//        if( Support::HostLanguages.count( hostLanguage ) == 0 )
//            throw std::invalid_argument("ERROR: host language " + hostLanguage
//            + " not supported.\n" );
//
//        if( hostLanguage.empty() == true )
//            throw std::invalid_argument("ERROR: host language is empty.\n" );
//    }
//
//    //adios-group
//    currentPosition = 0;
//
//    while( currentPosition != std::string::npos )
//    {
//        std::string xmlGroup;
//        GetSubString("<adios-group ", "</adios-group>", currentContent,
//        xmlGroup, currentPosition ); //Get all group contents
//
//        if( xmlGroup.empty() ) //no more groups to find
//            break;
//
//        //get group name
//        std::string::size_type groupPosition( 0 );
//        GetSubString( "<adios-group ", ">", xmlGroup, tag, groupPosition );
//        if( debugMode == true )
//        {
//            if( tag.size() < 2 )
//                throw std::invalid_argument( "ERROR: wrong tag " + tag + " in
//                adios-group\n" ); //check < or <=
//        }
//
//        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
//        GetPairsFromTag( xmlGroup, tag, pairs );
//        std::string groupName;
//
//        for( auto& pair : pairs )
//        {
//            if( pair.first == "name")
//                groupName = pair.second;
//        }
//
//        if( debugMode == true )
//        {
//            if( groupName.empty() )
//                throw std::invalid_argument( "ERROR: group name not found. \n"
//                );
//
//            if( groups.count( groupName ) == 1 ) //group exists
//                throw std::invalid_argument( "ERROR: group " + groupName + "
//                defined twice.\n" );
//        }
//
//        groups.emplace( groupName, Group( groupName, xmlGroup, transforms,
//        debugMode ) );
//
//        currentContent.erase( currentContent.find( xmlGroup ), xmlGroup.size()
//        );
//        currentPosition = 0;
//    }
//
//    //transport
//    //lambda function to check priority and iteration casting to unsigned int
//    auto lf_UIntCheck = []( const std::string method, const std::string
//    fieldStr, const std::string fieldName,
//                            const bool debugMode, int& field )
//    {
//        field = 0;
//        if( fieldStr.empty() == false )
//        {
//            field = std::stoi( fieldStr ); //throws invalid_argument
//
//            if( debugMode == true )
//            {
//                if( field < 0 )
//                    throw std::invalid_argument("ERROR: " + fieldName + " in
//                    transport " + method + " can't be negative\n" );
//            }
//        }
//    };
//
//    //this section will have to change, doing nothing for now
//    currentPosition = 0;
//    while( currentPosition != std::string::npos )
//    {
//        GetSubString( "<transport ", ">", currentContent, tag, currentPosition
//        );
//        if( tag.empty() ) break;
//        tag = tag.substr( 1, tag.size() - 2 ); //eliminate < >
//        pairs.clear();
//        GetPairsFromTag( currentContent, tag, pairs );
//
//        std::string groupName, method, priorityStr, iterationStr;
//        for( auto& pair : pairs )
//        {
//            if( pair.first == "group" )  groupName = pair.second;
//            else if( pair.first == "method" ) method = pair.second;
//            else if( pair.first == "priority" ) priorityStr = pair.second;
//            else if( pair.first == "iteration" ) iterationStr = pair.second;
//        }
//
//        auto itGroup = groups.find( groupName );
//        if( debugMode == true )
//        {
//            if( itGroup == groups.end() ) //not found
//                throw std::invalid_argument( "ERROR: in transport " + method +
//                " group " + groupName + " not found.\n" );
//        }
//
//        int priority, iteration;
//        lf_UIntCheck( method, priorityStr, "priority", debugMode, priority );
//        lf_UIntCheck( method, iterationStr, "iteration", debugMode, iteration
//        );
//        //here do something with the capsule
//    }
//}

// void InitXML( const std::string xmlConfigFile, const MPI_Comm mpiComm, const
// bool debugMode,
//              std::string& hostLanguage, std::vector<
//              std::shared_ptr<Transform> >& transforms,
//              std::map< std::string, Group >& groups )
//{
//    int xmlFileContentSize;
//    std::string xmlFileContent;
//
//    int rank;
//    MPI_Comm_rank( mpiComm, &rank );
//
//    if( rank == 0 ) //serial part
//    {
//        DumpFileToString( xmlConfigFile, xmlFileContent ); //in
//        ADIOSFunctions.h dumps all XML Config File to xmlFileContent
//        xmlFileContentSize = xmlFileContent.size( ) + 1; // add one for the
//        null character
//
//        MPI_Bcast( &xmlFileContentSize, 1, MPI_INT, 0, mpiComm ); //broadcast
//        size for allocation
//        MPI_Bcast( (char*)xmlFileContent.c_str(), xmlFileContentSize,
//        MPI_CHAR, 0, mpiComm ); //broadcast contents
//    }
//    else
//    {
//        MPI_Bcast( &xmlFileContentSize, 1, MPI_INT, 0, mpiComm  ); //receive
//        size
//
//        char* xmlFileContentMPI = new char[ xmlFileContentSize ]; //allocate
//        xml C-char
//        MPI_Bcast( xmlFileContentMPI, xmlFileContentSize, MPI_CHAR, 0, mpiComm
//        ); //receive xml C-char
//        xmlFileContent.assign( xmlFileContentMPI ); //copy to a string
//
//        delete []( xmlFileContentMPI ); //delete char* needed for MPI, might
//        add size is moving to C++14 for optimization, avoid memory leak
//    }
//
//    SetMembers( xmlFileContent,  mpiComm, debugMode, hostLanguage, transforms,
//    groups );
//}

std::size_t GetTotalSize(const std::vector<std::size_t> &dimensions)
{
    std::size_t product = 1;

    for (const auto dimension : dimensions)
    {
        product *= dimension;
    }

    return product;
}

void CreateDirectory(const std::string fullPath) noexcept
{
    auto lf_Mkdir = [](const std::string directory, struct stat &st) {
        if (stat(directory.c_str(), &st) == -1)
        {
            mkdir(directory.c_str(), 0777);
        }
    };

    auto directoryPosition = fullPath.find("/");

    if (fullPath[0] == '/' || fullPath[0] == '.')
    { // find the second '/'
        directoryPosition = fullPath.find("/", directoryPosition + 1);
    }

    struct stat st = {0};
    if (directoryPosition == fullPath.npos) // no subdirectories
    {
        lf_Mkdir(fullPath.c_str(), st);
        return;
    }

    std::string directory(fullPath.substr(0, directoryPosition));
    lf_Mkdir(directory.c_str(), st);

    while (directoryPosition != fullPath.npos)
    {
        directoryPosition = fullPath.find("/", directoryPosition + 1);
        directory = fullPath.substr(0, directoryPosition);
        lf_Mkdir(directory.c_str(), st);
    }
}

void SetTransformsHelper(const std::vector<std::string> &transformNames,
                         std::vector<std::shared_ptr<Transform>> &transforms,
                         const bool debugMode,
                         std::vector<short> &transformIndices,
                         std::vector<short> &parameters)
{
    // function to get a parameter from "method:parameter"
    auto lf_GetParameter = [](const std::string transformName,
                              std::string &transformMethod,
                              const bool debugMode) -> short {
        short parameter = -1;
        auto colonPosition = transformName.find(":");

        if (colonPosition != transformName.npos)
        {
            if (debugMode == true)
            {
                if (colonPosition == transformName.size() - 1)
                {
                    throw std::invalid_argument(
                        "ERROR: wrong format for transform " + transformName +
                        ", in call to SetTransform\n");
                }
            }

            transformMethod = transformName.substr(0, colonPosition);
            parameter = std::stoi(
                transformName.substr(colonPosition + 1)); // need to test
        }
        return parameter;
    };

    // Get transform index from transforms, if not found return -1
    auto lf_GetTransformIndex =
        [](const std::string transformMethod,
           const std::vector<std::shared_ptr<Transform>> &transforms) -> short {
        short transformIndex = -1;
        for (unsigned int i = 0; i < transforms.size(); ++i)
        {
            if (transforms[i]->m_Method == transformMethod)
            {
                transformIndex = i;
                break;
            }
        }
        return transformIndex;
    };

    // BODY of FUNCTION STARTS HERE
    for (const std::string transformName : transformNames)
    {
        std::string transformMethod(transformName);
        short parameter =
            lf_GetParameter(transformName, transformMethod,
                            debugMode); // from transform = "method:parameter"
        short transformIndex =
            lf_GetTransformIndex(transformMethod, transforms);

        if (transformIndex == -1) // not found, then create a new transform
        {
            if (transformMethod == "bzip2")
            {
#ifdef HAVE_BZIP2
                transforms.push_back(std::make_shared<CBZIP2>());
#endif
            }

            transformIndex = static_cast<short>(transforms.size() - 1);
        }
        transformIndices.push_back(transformIndex);
        parameters.push_back(parameter);
    }
}

std::map<std::string, std::string>
BuildParametersMap(const std::vector<std::string> &parameters,
                   const bool debugMode)
{
    auto lf_GetFieldValue = [](const std::string parameter, std::string &field,
                               std::string &value, const bool debugMode) {
        auto equalPosition = parameter.find("=");

        if (debugMode == true)
        {
            if (equalPosition == parameter.npos)
            {
                throw std::invalid_argument(
                    "ERROR: wrong format for parameter " + parameter +
                    ", format must be field=value \n");
            }

            if (equalPosition == parameter.size() - 1)
            {
                throw std::invalid_argument("ERROR: empty value in parameter " +
                                            parameter +
                                            ", format must be field=value \n");
            }
        }

        field = parameter.substr(0, equalPosition);
        value = parameter.substr(equalPosition + 1); // need to test
    };

    // BODY OF FUNCTION STARTS HERE
    std::map<std::string, std::string> parametersOutput;

    for (const auto parameter : parameters)
    {
        std::string field, value;
        lf_GetFieldValue(parameter, field, value, debugMode);

        if (debugMode == true)
        {
            if (parametersOutput.count(field) == 1)
            {
                throw std::invalid_argument(
                    "ERROR: parameter " + field +
                    " already exists, must be unique\n");
            }
        }

        parametersOutput[field] = value;
    }

    return parametersOutput;
}

std::vector<int> CSVToVectorInt(const std::string csv)
{
    std::vector<int> numbers;
    if (csv.empty())
    {
        return numbers;
    }

    if (csv.find(",") == csv.npos) // check if no commas, one int
    {
        numbers.push_back(std::stoi(csv)); // might need to be checked
    }
    else
    {
        int count = std::count(csv.begin(), csv.end(), ',');
        numbers.reserve(count);

        std::istringstream csvSS(csv);
        std::string value;
        while (std::getline(csvSS, value, ',')) // need to test
        {
            numbers.push_back(std::stoi(csv));
        }
    }

    return numbers;
}

void ConvertUint64VectorToSizetVector(const std::vector<std::uint64_t> &in,
                                      std::vector<std::size_t> &out)
{
    out.clear();
    out.reserve(in.size());
    for (const auto inElement : in)
    {
        out.push_back(static_cast<std::size_t>(inElement));
    }
}

bool CheckBufferAllocation(const std::size_t newSize, const float growthFactor,
                           const std::size_t maxBufferSize,
                           std::vector<char> &buffer)
{
    // Check if data in buffer needs to be reallocated
    const std::size_t requiredDataSize =
        buffer.size() + newSize + 100; // adding some bytes for tolerance
    // might need to write payload in batches
    bool doTransportsFlush = (requiredDataSize > maxBufferSize) ? true : false;

    if (GrowBuffer(requiredDataSize, growthFactor, buffer) == -1)
    {
        doTransportsFlush = true;
    }

    return doTransportsFlush;
}

int GrowBuffer(const std::size_t incomingDataSize, const float growthFactor,
               std::vector<char> &buffer)
{
    const std::size_t currentCapacity = buffer.capacity();
    const std::size_t availableSpace = currentCapacity - buffer.size();
    const double gf = static_cast<double>(growthFactor);

    if (incomingDataSize > availableSpace)
    {
        const std::size_t neededCapacity = incomingDataSize + buffer.size();
        const double numerator = std::log(static_cast<double>(neededCapacity) /
                                          static_cast<double>(currentCapacity));
        const double denominator = std::log(gf);

        double n = std::ceil(numerator / denominator);
        const std::size_t newSize = static_cast<std::size_t>(
            std::ceil(std::pow(gf, n) * currentCapacity));

        try
        {
            buffer.reserve(newSize);
        }
        catch (std::bad_alloc &e)
        {
            return -1;
        }

        return 1;
    }
    return 0;
}

bool IsLittleEndian() noexcept
{
    uint16_t hexa = 0x1234;
    return *reinterpret_cast<uint8_t *>(&hexa) != 0x12; // NOLINT
}

} // namespace adios
