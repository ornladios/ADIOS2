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

namespace adios2
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
void InitXML(const std::string configXML, const MPI_Comm mpiComm,
             const bool debugMode,
             std::vector<std::shared_ptr<Transform>> &transforms,
             std::map<std::string, IO> &ios);
}

#endif /* ADIOS2_HELPER_ADIOSXML_H_ */
