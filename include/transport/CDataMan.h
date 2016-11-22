/*
 * CDataMan.h
 *
 *  Created on: Nov 15, 2016
 *      Author: wfg
 */

#ifndef CDATAMAN_H_
#define CDATAMAN_H_

#include "core/CTransport.h"

//here include any external library headers only if needed by the declarations in this file, we can figure out linking them later

namespace adios
{

class CDataMan : public CTransport
{


public:

    CDataMan( MPI_Comm mpiComm, const bool debugMode );

    ~CDataMan( );

    /**
     * Open your communication socket ?
     * @param streamName message name ?
     * @param accessMode w or write, r or read
     */
    void Open( const std::string streamName, const std::string accessMode );

    /**
     * You might not need this one since Buffer has all data, we might turn it on for future reference
     * @param buffer
     */
    void SetBuffer( const std::vector<char>& buffer );

    /**
     * Here you will receive a reference (not a copy) to the variable being written as raw data ready to be sent.
     * Your magic will go inside this function. From ADIOS it's just a single call per variable.
     * @param buffer contains a reference to the raw data variable. This backtraces to ADIOS.Write( group, variableName, * value )
     */
    void Write( std::vector<char>& buffer );

    /**
     * Required to terminate your acquire system resource (file, socket, stream, etc.) if hanging to avoid memory leaks
     */
    void Close( );


// FILE* m_File; //here put your unique, particular form of communication. See CPOSIX and CFStream for files.
// You have your own structs, objects etc. put them in protected (if inherited) or private space
// We can also work on initializing them in the constructor

//protected:

//private:


};












} //end namespace


#endif /* CDATAMAN_H_ */
