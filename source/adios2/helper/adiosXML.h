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
/// \endcond

#include "adios2/core/IO.h"
#include "adios2/core/Operator.h"

#include <pugixml.hpp>

namespace adios2
{
namespace helper
{

/**
 * Get and check XML Document
 * @param xmlContents string with xml contents
 * @param debugMode true: check if ill-formed, false: no-check
 * @param hint improve exception message for debugging
 * @return  pugi compatible xml document
 * @exception std::invalid_argument in debugMode if xml is ill-formed
 */
pugi::xml_document XMLDocument(const std::string &xmlContents,
                               const bool debugMode, const std::string hint);

/**
 * Get xml tag "element node" <nodeName> ... </nodeName> from xml document
 * @param nodeName tag element node to look for
 * @param document xml string source for finding nodeName
 * @param debugMode true: check if nodeName exist, false: no-check
 * @param hint improve exception message for debugging
 * @param isUnique true: nodeName must be unique in XML document, false:
 * multiple elements can have the same name
 * @return pugi node for nodeName
 * @exception std::invalid_argument in debugMode if nodeName not found
 */
pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_document &document, const bool debugMode,
                       const std::string hint, const bool isUnique = false);

/**
 * Version that gets a node from inside another node
 * @param nodeName tag element node to look for
 * @param upperNode xml node element for finding nodeName
 * @param debugMode true: check if nodeName exist, false: no-check
 * @param hint improve exception message for debugging
 * @param isUnique true: nodeName must be unique in XML document, false:
 * multiple elements can have the same name
 * @return pugi node for nodeName
 * @exception std::invalid_argument in debugMode if nodeName not found or is not
 * unique (if isUnique=true)
 */
pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_node &upperNode, const bool debugMode,
                       const std::string hint, const bool isUnique = false);

/**
 * Gets a node attribute e.g. name in io <io name="testIO">
 * @param attributeName
 * @param node
 * @param debugMode
 * @param hint
 * @return
 */
pugi::xml_attribute XMLAttribute(const std::string attributeName,
                                 const pugi::xml_node &node,
                                 const bool debugMode, const std::string hint);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
