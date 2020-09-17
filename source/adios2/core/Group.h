/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Group.h template implementations with fix types and specializations
 *
 *  Created on: August 25, 2020
 *      Author: Dmitry Ganyushin ganyushindi@ornl.gov
 */
#ifndef ADIOS2_CORE_GROUP_H
#define ADIOS2_CORE_GROUP_H
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include <map>
#include <set>
#include <string>
#include <vector>
namespace adios2
{
namespace core
{
/** used for Variables and Attributes, name, type, type-index */
using DataMap =
    std::unordered_map<std::string, std::pair<std::string, unsigned int>>;
class Group
{
private:
    struct TreeMap
    {
        std::map<std::string, std::set<std::string>> treeMap;
    };
    /** current path of the Group object */
    std::string currentPath;
    /** demiliter symbol between groups */
    char groupDelimiter;
    /** shared pointer to a map representing the tree structure */
    std::shared_ptr<TreeMap> mapPtr = nullptr;

public:
    /**
     * @brief Constructor called from IO factory class GetGroup function.
     * Not to be used directly in applications.
     * @param current path
     * @param a separate symbol
     * @param IO reference object to IO object that owns the current Group
     * object
     */
    Group(std::string path, char delimiter, IO &m_IO);
    /** copy constructor */
    Group(const Group &G);
    /** destructor */
    ~Group();
    /**
     * @brief Builds map that represents tree structure from m_Variable and
     * m_Attributes from IO class
     * @param
     */
    void BuildTree();
    /**
     * @brief Prints map that represents tree structure
     * @param
     */
    void PrintTree();
    /**
     * @brief returns available groups on the path set
     * @param
     * @return vector of strings
     */
    std::vector<std::string> AvailableGroups();
    /**
     * @brief returns available variables on the path set
     * @param
     * @return vector of strings
     */
    std::vector<std::string> AvailableVariables();
    /**
     * @brief returns available attributes on the path set
     * @param
     * @return vector of strings
     */
    std::vector<std::string> AvailableAttributes();
    /**
     * @brief returns the current path
     * @param
     * @return current path as a string
     */
    std::string InquirePath();
    /**
     * @brief returns a new group object
     * @param name of the group
     * @return new group object
     */
    Group *OpenGroup(std::string groupName);
    /**
     * @brief set the path, points to a particular node on the tree
     * @param next possible path extension
     */
    void setPath(std::string path);
    /**
     * @brief set the delimiter for group connection in a string representation
     * @param delimiter symbol
     */
    void setDelimiter(char delimiter);
    /**
     * @brief returns  a reference to the map representing the tree stucture
     * @param delimiter symbol
     */
    std::map<std::string, std::set<std::string>> &getTreeMap();
    /** reference to object that created current Group */
    IO &m_IO;
    /**
     * @brief Gets an existing variable of primitive type by name. A wrapper for
     * the corresponding function of the IO class
     * @param name of variable to be retrieved
     * @return pointer to an existing variable in current IO, nullptr if not
     * found
     */
    template <class T>
    Variable<T> *InquireVariable(const std::string &name) noexcept;
    /**
     * Gets an existing attribute of primitive type by name. A wrapper for
     * the corresponding function of the IO class
     * @param name of attribute to be retrieved
     * @return pointer to an existing attribute in current IO, nullptr if not
     * found
     */
    template <class T>
    Attribute<T> *InquireAttribute(const std::string &name,
                                   const std::string &variableName = "",
                                   const std::string separator = "/") noexcept;
    /**
     * @brief Returns the type of an existing variable as an string. A wrapper
     * for the corresponding function of the IO class
     * @param name input variable name
     * @return type primitive type
     */
    std::string InquireVariableType(const std::string &name) const noexcept;

    /**
     * Overload that accepts a const iterator into the m_Variables map if found.
     * A wrapper for the corresponding function of the IO class
     * @param itVariable
     * @return type primitive type
     */
    std::string InquireVariableType(
        const DataMap::const_iterator itVariable) const noexcept;

    /**
     * @brief Returns the type of an existing attribute as an string. A wrapper
     * for the corresponding function of the IO class
     * @param name input attribute name
     * @return type if found returns type as string, otherwise an empty string
     */
    std::string
    InquireAttributeType(const std::string &name,
                         const std::string &variableName = "",
                         const std::string separator = "/") const noexcept;
};
// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template Variable<T> *Group::InquireVariable<T>(                    \
        const std::string &name) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_CORE_GROUP_H
