/*
 * CSZIP.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CSZIP_H_
#define CSZIP_H_


#include <szlib.h> //szip library header


#include "core/CTransform.h"


namespace adios
{


class CSZIP : public CTransform
{

public:

    /**
     * CSZIP constructor
     * @param compressionLevel
     * @param variable
     */
    CSZIP( const unsigned int compressionLevel, CVariableBase& variable );

    ~CSZIP( );

    void Compress( ) const;

    void Decompress( ) const;

};


} //end namespace



#endif /* CSZIP_H_ */
