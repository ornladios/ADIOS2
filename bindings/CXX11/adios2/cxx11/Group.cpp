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
#include "adios2/core/Group.h"

namespace adios2
{
Group::Group(core::Group *group) : m_Group(group) {}

Group Group::OpenGroup(std::string group_name)
{
    auto m = m_Group->OpenGroup(group_name);
    return Group(m);
}
void Group::PrintTree()
{
    m_Group->PrintTree();
    return;
}

void Group::BuildTree()
{
    m_Group->BuildTree();
    return;
}
std::vector<std::string> Group::AvailableVariables()
{
    return m_Group->AvailableVariables();
}
std::vector<std::string> Group::AvailableAttributes()
{
    return m_Group->AvailableAttributes();
}
std::vector<std::string> Group::AvailableGroups()
{
    return m_Group->AvailableGroups();
}

std::map<std::string, std::set<std::string>> &Group::getTreeMap()
{
    return m_Group->getTreeMap();
}

std::string Group::InquirePath() { return m_Group->InquirePath(); }

void Group::setPath(std::string path) { m_Group->setPath(path); }
std::string Group::VariableType(const std::string &name) const
{
    helper::CheckForNullptr(m_Group, "in call to IO::VariableType");
    return m_Group->InquireVariableType(name);
}

std::string Group::AttributeType(const std::string &name) const
{
    helper::CheckForNullptr(m_Group, "in call to IO::AttributeType");
    return m_Group->InquireAttributeType(name);
}
Group::~Group() { m_Group->~Group(); }
// Explicit declaration of the public template methods
// Limits the types
#define declare_template_instantiation(T)                                      \
    template Variable<T> Group::InquireVariable<T>(const std::string &);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
} // end of namespace