/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Group.cpp :
 *
 *  Created on: August 25, 2020
 *      Author: Dmitry Ganyushin ganyushindi@ornl.gov
 */
#include "Group.h"
#include "Group.tcc"
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
namespace adios2
{
namespace core
{
std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}
void Group::setPath(std::string path) { currentPath = path; }
void Group::setDelimiter(char delimiter) { groupDelimiter = delimiter; }

Group::Group(std::string path, char delimiter, IO &io)
: currentPath(path), groupDelimiter(delimiter), m_IO(io)
{
    if (mapPtr == nullptr)
    {
        mapPtr = std::shared_ptr<TreeMap>(new TreeMap());
    }
}
// copy constructor
Group::Group(const Group &G)
: currentPath(G.currentPath), groupDelimiter(G.groupDelimiter), m_IO(G.m_IO)
{
    mapPtr = G.mapPtr;
}
Group *Group::InquireGroup(std::string groupName)
{
    Group *g_out = new Group(currentPath + groupDelimiter + groupName,
                             this->groupDelimiter, this->m_IO);
    g_out->mapPtr = this->mapPtr;
    return g_out;
}
void Group::PrintTree()
{
    for (auto k : mapPtr->treeMap)
    {
        std::cout << k.first << "=>";
        for (auto v : k.second)
            std::cout << v << " ";
        std::cout << std::endl;
    }
}

void Group::BuildTree()
{
    const core::VarMap &variables = m_IO.GetVariables();
    for (const auto &variablePair : variables)
    {
        std::vector<std::string> tokens =
            split(variablePair.first, groupDelimiter);

        if (tokens.size() == 0)
        {
            // record = "group". Handled by default case
        }
        else if (tokens.size() == 1)
        {
            // case record = "/group1" or "group/"
        }
        if (tokens.size() > 1)
        {
            std::string key = tokens[0];
            for (int level = 1; level < tokens.size(); level++)
            {
                std::string value = tokens[level];
                // get previous vector
                std::set<std::string> val = mapPtr->treeMap[key];
                // modify it
                val.insert(value);
                mapPtr->treeMap[key] = val;
                key += groupDelimiter + tokens[level];
            }
        }
    }
    const core::AttrMap &attributes = m_IO.GetAttributes();
    for (const auto &attributePair : attributes)
    {
        std::vector<std::string> tokens =
            split(attributePair.first, groupDelimiter);

        if (tokens.size() == 0)
        {
            // record = "group". Handled by default case
        }
        else if (tokens.size() == 1)
        {
            // case record = "/group1" or "group/"
        }
        if (tokens.size() > 1)
        {
            std::string key = tokens[0];
            for (int level = 1; level < tokens.size(); level++)
            {
                std::string value = tokens[level];
                // get previous vector
                std::set<std::string> val = mapPtr->treeMap[key];
                // modify it
                val.insert(value);
                mapPtr->treeMap[key] = val;
                key += groupDelimiter + tokens[level];
            }
        }
    }
}
std::vector<std::string> Group::AvailableVariables()
{
    // look into map
    std::set<std::string> val = mapPtr->treeMap[currentPath];
    // TODODG check that currentPath exists
    std::vector<std::string> available_variables;
    for (auto v : val)
    {
        if (mapPtr->treeMap.find(currentPath + groupDelimiter + v) ==
            mapPtr->treeMap.end())
        {
            const core::VarMap &variables = m_IO.GetVariables();

            if (variables.find(currentPath + groupDelimiter + v) !=
                variables.end())
            {
                available_variables.push_back(v);
            }
        }
    }

    return available_variables;
}

std::vector<std::string> Group::AvailableAttributes()
{
    // look into map
    std::set<std::string> val = mapPtr->treeMap[currentPath];
    // TODODG check that currentPath exists
    std::vector<std::string> available_attributes;
    for (auto v : val)
    {
        if (mapPtr->treeMap.find(currentPath + groupDelimiter + v) ==
            mapPtr->treeMap.end())
        {
            const core::AttrMap &attributes = m_IO.GetAttributes();
            if (attributes.find(currentPath + groupDelimiter + v) !=
                attributes.end())
            {
                available_attributes.push_back(v);
            }
        }
    }

    return available_attributes;
}

std::vector<std::string> Group::AvailableGroups()
{

    std::vector<std::string> available_groups;
    std::set<std::string> val = mapPtr->treeMap[currentPath];

    for (auto v : val)
    {
        if (mapPtr->treeMap.find(currentPath + groupDelimiter + v) !=
            mapPtr->treeMap.end())
            available_groups.push_back(v);
    }

    return available_groups;
}

std::map<std::string, std::set<std::string>> &Group::getTreeMap()
{
    std::map<std::string, std::set<std::string>> &tree = mapPtr->treeMap;
    return tree;
}

std::string Group::InquirePath() { return currentPath; }
Group::~Group() = default;
DataType Group::InquireVariableType(const std::string &name) const noexcept
{

    return m_IO.InquireVariableType(currentPath + groupDelimiter + name);
}

DataType Group::InquireAttributeType(const std::string &name,
                                     const std::string &variableName,
                                     const std::string separator) const noexcept
{
    return m_IO.InquireAttributeType(name, variableName, separator);
}
// Explicitly instantiate the necessary public template implementations
#define define_template_instantiation(T)                                       \
    template Variable<T> *Group::InquireVariable<T>(                           \
        const std::string &) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(define_template_instantiation)
#undef define_template_instatiation

} // end namespace core
} // end namespace adios2
