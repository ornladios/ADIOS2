/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPReader.h
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#ifndef BPFILEREADER_H_
#define BPFILEREADER_H_

#include <iostream> //this must go away

#include "core/Engine.h"

#include "capsule/heap/STLVector.h"

namespace adios
{

class BPFileReader : public Engine
{

public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param hostLanguage
     */
    BPFileReader(ADIOS &adios, const std::string &name,
                 const std::string accessMode, MPI_Comm mpiComm,
                 const Method &method);

    virtual ~BPFileReader() = default;

    Variable<void> *InquireVariable(const std::string &variableName,
                                    const bool readIn = true);

    Variable<char> *InquireVariableChar(const std::string &variableName,
                                        const bool readIn = true);

    Variable<unsigned char> *
    InquireVariableUChar(const std::string &variableName,
                         const bool readIn = true);

    Variable<short> *InquireVariableShort(const std::string &variableName,
                                          const bool readIn = true);

    Variable<unsigned short> *
    InquireVariableUShort(const std::string &variableName,
                          const bool readIn = true);

    Variable<int> *InquireVariableInt(const std::string &variableName,
                                      const bool readIn = true);

    Variable<unsigned int> *InquireVariableUInt(const std::string &variableName,
                                                const bool readIn = true);

    Variable<long int> *InquireVariableLInt(const std::string &variableName,
                                            const bool readIn = true);

    Variable<unsigned long int> *
    InquireVariableULInt(const std::string &variableName,
                         const bool readIn = true);

    Variable<long long int> *
    InquireVariableLLInt(const std::string &variableName,
                         const bool readIn = true);

    Variable<unsigned long long int> *
    InquireVariableULLInt(const std::string &variableName,
                          const bool readIn = true);

    Variable<float> *InquireVariableFloat(const std::string &variableName,
                                          const bool readIn = true);

    Variable<double> *InquireVariableDouble(const std::string &variableName,
                                            const bool readIn = true);
    Variable<long double> *
    InquireVariableLDouble(const std::string &variableName,
                           const bool readIn = true);

    Variable<std::complex<float>> *
    InquireVariableCFloat(const std::string &variableName,
                          const bool readIn = true);

    Variable<std::complex<double>> *
    InquireVariableCDouble(const std::string &variableName,
                           const bool readIn = true);

    Variable<std::complex<long double>> *
    InquireVariableCLDouble(const std::string &variableName,
                            const bool readIn = true);

    /**
     * Not implemented
     * @param name
     * @param readIn
     * @return
     */
    VariableCompound *InquireVariableCompound(const std::string &variableName,
                                              const bool readIn = true);

    void Close(const int transportIndex = -1);

private:
    capsule::STLVector
        m_Buffer; ///< heap capsule, contains data and metadata buffers
    // format::BP1Writer m_BP1Writer; ///< format object will provide the
    // required
    // BP functionality to be applied on m_Buffer and m_Transports

    void Init(); ///< calls InitCapsules and InitTransports based on Method,
                 /// called from constructor

    void InitTransports(); ///< from Transports

    std::string
    GetMdtmParameter(const std::string parameter,
                     const std::map<std::string, std::string> &mdtmParameters);

    template <class T>
    Variable<T> *InquireVariableCommon(const std::string &name,
                                       const bool readIn)
    {
        std::cout << "Hello BPReaderCommon\n";

        // here read variable metadata (dimensions, type, etc.)...then create a
        // Variable like below:
        // Variable<T>& variable = m_ADIOS.DefineVariable<T>( m_Name + "/" +
        // name, )
        // return &variable; //return address if success
        return nullptr; // on failure
    }
};

} // end namespace adios

#endif /* BPFILEREADER_H_ */
