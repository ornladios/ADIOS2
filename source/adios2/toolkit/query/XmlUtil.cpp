#include "XmlUtil.h"
#include <stdexcept>

namespace adios2
{
namespace query
{
namespace XmlUtil
{
pugi::xml_document XMLDocument(const std::string &xmlContents)
{
    pugi::xml_document document;
    auto parse_result = document.load_buffer_inplace(
        const_cast<char *>(xmlContents.data()), xmlContents.size());

    if (!parse_result)
    {
        throw std::invalid_argument("ERROR: XML parse error: " +
                                    std::string(parse_result.description()));
    }

    return document;
}

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_document &xmlDocument,
                       const bool isMandatory, const bool isUnique)
{
    const pugi::xml_node node = xmlDocument.child(nodeName.c_str());

    if (isMandatory && !node)
    {
        throw std::invalid_argument("ERROR: XML: no <" + nodeName +
                                    "> element found");
    }

    if (isUnique)
    {
        const size_t nodes =
            std::distance(xmlDocument.children(nodeName.c_str()).begin(),
                          xmlDocument.children(nodeName.c_str()).end());
        if (nodes > 1)
        {
            throw std::invalid_argument("ERROR: XML only one <" + nodeName +
                                        "> element can exist inside " +
                                        std::string(xmlDocument.name()));
        }
    }

    return node;
}

pugi::xml_node XMLNode(const std::string nodeName,
                       const pugi::xml_node &upperNode, const bool isMandatory,
                       const bool isUnique)
{
    const pugi::xml_node node = upperNode.child(nodeName.c_str());

    if (isMandatory && !node)
    {
        throw std::invalid_argument(
            "ERROR: XML: no <" + nodeName + "> element found, inside <" +
            std::string(upperNode.name()) + "> element ");
    }

    if (isUnique)
    {
        const size_t nodes =
            std::distance(upperNode.children(nodeName.c_str()).begin(),
                          upperNode.children(nodeName.c_str()).end());
        if (nodes > 1)
            throw std::invalid_argument("ERROR: XML only one <" + nodeName +
                                        "> element can exist inside <" +
                                        std::string(upperNode.name()) +
                                        "> element. ");
    }
    return node;
}

pugi::xml_attribute XMLAttribute(const std::string attributeName,
                                 const pugi::xml_node &node,
                                 const bool isMandatory)
{
    const pugi::xml_attribute attribute = node.attribute(attributeName.c_str());

    if (isMandatory && !attribute)
    {
        const std::string nodeName(node.name());
        throw std::invalid_argument("ERROR: XML: No attribute " +
                                    attributeName + " found on <" + nodeName +
                                    "> element");
    }
    return attribute;
}

} // namespace XmlUtil
} //  namespace query
} // namespace adios2
