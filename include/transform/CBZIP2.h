/*
 * CBZIP2.h
 *
 *  Created on: Oct 17, 2016
 *      Author: wfg
 */

#ifndef CBZIP2_H_
#define CBZIP2_H_

#ifdef BZIP2
#include "bzip2.h" //bzip2 library header
#endif

#include "CTransform.h"


namespace adios
{


class CBZIP2 : public CTransform
{

public:

    const std::string m_Method; ///< name of the transformation method
    const unsigned int m_CompressionLevel; ///< depends on library implementation
    CVariable& m_Variable; ///< variable to be transformed

    /**
     * Initialize parent method
     * @param variable
     */
    CBZIP2( CVariable& variable );

    ~CBZIP2( );

    void WriteTransform( );

    void ReadTransform( );

    void GetCompressedLength( ) const;

    void GetExpandedLength( ) const;

};


} //end namespace



#endif /* CBZIP2_H_ */
