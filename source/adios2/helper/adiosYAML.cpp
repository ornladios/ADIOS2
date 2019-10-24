/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosYAML.h basic YAML parsing functionality for ADIOS config file schema
 *
 *  Created on: Oct 24, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosYAML.h"

#include <yaml-cpp/yaml.h>

namespace adios2
{
namespace helper
{

void ParseConfigYAML(
    core::ADIOS &adios, const std::string &configFileXML,
    std::map<std::string, core::IO> &ios,
    std::map<std::string, std::shared_ptr<core::Operator>> &operators)
{
}

} // end namespace helper
} // end namespace adios2
