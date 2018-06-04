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

namespace adios2
{
namespace helper
{

/**
 * Called inside the ADIOS XML constructors to get contents from file,
 * broadcast and fill transforms and ios
 * @param configXMLFile
 * @param mpiComm
 * @param debugMode
 * @param transforms
 * @param ios
 */
void InitXML(const std::string configXML, MPI_Comm mpiComm,
             const std::string hostLanguage, const bool debugMode,
             std::map<std::string, std::shared_ptr<core::Operator>> &transforms,
             std::map<std::string, core::IO> &ios);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
