/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosXML.h basic XML parsing functionality for ADIOS config file schema
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSXML_H_
#define ADIOS2_HELPER_ADIOSXML_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <memory> //std::shared_ptr
#include <string>
#include <utility> //std::pair
#include <vector>
/// \endcond

#include "adios2/core/IO.h"
#include "adios2/core/Transform.h"

namespace adios
{

struct TagXML
{
    std::string Header;
    std::string Elements;
    bool IsFull;
};

void RemoveCommentsXML(std::string &currentContent) noexcept;

TagXML GetTagXML(const std::string tagName, const std::string &content,
                 std::string::size_type &position);

std::string::size_type
GetStringPositionXML(const std::string input, const std::string &content,
                     const std::string::size_type &startPosition) noexcept;

Params GetTagAttributesXML(const std::string &tagHeader);

/**
 * Called inside the ADIOS XML constructors to get contents from file,
 * broadcast and fill transforms and ios
 * @param configXMLFile
 * @param mpiComm
 * @param debugMode
 * @param transforms
 * @param ios
 */
void InitXML(const std::string configXML, const MPI_Comm mpiComm,
             const bool debugMode,
             std::vector<std::shared_ptr<Transform>> &transforms,
             std::map<std::string, IO> &ios);

void InitIOXML(const TagXML &ioXML, const MPI_Comm mpiComm,
               const bool debugMode,
               std::vector<std::shared_ptr<Transform>> &transforms,
               std::map<std::string, IO> &ios);

void InitEngineXML(const TagXML &engineXML, const bool debugMode, IO &io);

void InitTransportXML(const TagXML &transportXML, const bool debugMode, IO &io);

Params ParseParamsXML(const std::string &tagContents, const bool debugMode);
}

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
