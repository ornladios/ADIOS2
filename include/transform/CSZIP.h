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
     * Initialize parent method
     * @param variable
     */
    CSZIP( const unsigned int compressionLevel, CVariable& variable );

    ~CSZIP( );

    void WriteTransform( );

    void ReadTransform( );

    void GetCompressedLength( ) const;

    void GetExpandedLength( ) const;

};


} //end namespace



#endif /* CSZIP_H_ */
