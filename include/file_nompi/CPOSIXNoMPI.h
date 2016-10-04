/*
 * CPOSIXNoMPI.h
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */

#ifndef CPOSIXNOMPI_H_
#define CPOSIXNOMPI_H_


#include "file_nompi/CFileNoMPI.h"


namespace adios
{


class CPOSIXNoMPI : public CFileNoMPI
{

public:

    CPOSIXNoMPI( const std::string fileName, const std::string fileType, const SMetadata& metadata );

    ~CPOSIXNoMPI( );

    void Open( const std::string fileName, const std::string groupName, const std::string accessMode );

    unsigned long int GroupSize( const std::string fileName, const std::string groupName ) const;

    void Write( const std::string fileName, const std::string variableName );

    void Close( );

};


} //end namespace



#endif /* CPOSIXNOMPI_H_ */
