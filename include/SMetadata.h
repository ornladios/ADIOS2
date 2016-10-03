/*
 * SMetadata.h : contains metadata structs
 *
 *  Created on: Sep 30, 2016
 *      Author: wfg
 */

#ifndef SMETADATA_H_
#define SMETADATA_H_

namespace adios
{
/**
 * Metadata picked up from the XML config file, members can't be const as they are know after reading the config XML file
 */
struct SMetadata
{
    std::string hostLanguage; ///< Supported: C, C++, Fortran
    ///add more Metadata?





};


} //end namespace

#endif /* SMETADATA_H_ */
