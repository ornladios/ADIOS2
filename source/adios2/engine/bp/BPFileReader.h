/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.h
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP_BPFILEREADER_H_
#define ADIOS2_ENGINE_BP_BPFILEREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

namespace adios2
{

class BPFileReader : public Engine
{

public:
    /**
     *
     */
    BPFileReader(IO &io, const std::string &name, const OpenMode openMode,
                 MPI_Comm mpiComm);

    virtual ~BPFileReader() = default;

    void Close(const int transportIndex = -1);

private:
    void Init(); ///< calls InitCapsules and InitTransports based on Method,
                 /// called from constructor

    void InitTransports(); ///< from Transports

    VariableBase *InquireVariableUnknown(const std::string &variableName,
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

#endif /* ADIOS2_ENGINE_BP_BPFILEREADER_H_ */
