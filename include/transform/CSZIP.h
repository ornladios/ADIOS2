/*
 * CSZIP.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CSZIP_H_
#define CSZIP_H_



#include <szlib.h> //szip library header

#include "CTransform.h"


namespace adios
{


class CSZIP : public CTransform
{

public:

    const std::string m_Method; ///< name of the transformation method
    const unsigned int m_CompressionLevel; ///< depends on library implementation
    CVariable& m_Variable; ///< variable to be transformed

    /**
     * Initialize parent method
     * @param variable
     */
    CSZIP( CVariable& variable );

    ~CSZIP( );

    void WriteTransform( );

    void ReadTransform( );

    void GetCompressedLength( ) const;

    void GetExpandedLength( ) const;

};


} //end namespace




#endif /* CSZIP_H_ */
