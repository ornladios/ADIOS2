/*
 * CBZIP2.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CBZIP2_H_
#define CBZIP2_H_


#include <bzip2.h> //bzip2 library header


#include "core/CTransform.h"


namespace adios
{


class CBZIP2 : public CTransform
{

public:

    /**
     * Initialize parent method
     * @param compressionLevel
     * @param variable
     */
    CBZIP2( const unsigned int compressionLevel, CVariable& variable );

    ~CBZIP2( );

    void WriteTransform( );

    void ReadTransform( );

    void GetCompressedLength( ) const;

    void GetExpandedLength( ) const;

};


} //end namespace



#endif /* CBZIP2_H_ */
