#ifndef ADIOS2_QUERY_XMLUTIL_H
#define ADIOS2_QUERY_XMLUTIL_H

#include <pugixml.hpp>

namespace adios2
{
namespace query
{
namespace XmlUtil
{

pugi::xml_attribute XMLAttribute(const std::string attributeName,
                                 const pugi::xml_node &node,
                                 const bool isMandatory = true);

// adios2::Dims split (const std::string &s, char delim);

pugi::xml_document XMLDocument(const std::string &xmlContents);

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_document &xmlDocument,
                       const bool isMandatory = true,
                       const bool isUnique = false);

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_node &upperNode,
                       const bool isMandatory = true,
                       const bool isUnique = false);

} // namespace xmlUtil
}
}

#endif
