/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.h
 * Class to read files using old adios 1.x library.
 * It requires adios 1.x installed
 *
 *  Created on: Mar 27, 2017
 *      Author: pnb
 */

#ifndef ADIOS1READER_H_
#define ADIOS1READER_H_

#include <iostream> //this must go away

#include "core/Engine.h"

// supported capsules
#include "capsule/heap/STLVector.h"

namespace adios
{

#ifndef ADIOS_HAVE_MPI
#define _NOMPI 1
#endif
#include "adios_read_v2.h" // this is adios 1.x header file
#ifndef ADIOS_HAVE_MPI
#undef _NOMPI
#endif

class ADIOS1Reader : public Engine
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
    ADIOS1Reader(ADIOS &adios, const std::string &name,
                 const std::string accessMode, MPI_Comm mpiComm,
                 const Method &method);

    ~ADIOS1Reader();

    Variable<void> *InquireVariable(const std::string &name,
                                    const bool readIn = true);
    Variable<char> *InquireVariableChar(const std::string &name,
                                        const bool readIn = true);
    Variable<unsigned char> *InquireVariableUChar(const std::string &name,
                                                  const bool readIn = true);
    Variable<short> *InquireVariableShort(const std::string &name,
                                          const bool readIn = true);
    Variable<unsigned short> *InquireVariableUShort(const std::string &name,
                                                    const bool readIn = true);
    Variable<int> *InquireVariableInt(const std::string &name,
                                      const bool readIn = true);
    Variable<unsigned int> *InquireVariableUInt(const std::string &name,
                                                const bool readIn = true);
    Variable<long int> *InquireVariableLInt(const std::string &name,
                                            const bool readIn = true);
    Variable<unsigned long int> *InquireVariableULInt(const std::string &name,
                                                      const bool readIn = true);
    Variable<long long int> *InquireVariableLLInt(const std::string &name,
                                                  const bool readIn = true);
    Variable<unsigned long long int> *
    InquireVariableULLInt(const std::string &name, const bool readIn = true);
    Variable<float> *InquireVariableFloat(const std::string &name,
                                          const bool readIn = true);
    Variable<double> *InquireVariableDouble(const std::string &name,
                                            const bool readIn = true);
    Variable<long double> *InquireVariableLDouble(const std::string &name,
                                                  const bool readIn = true);
    Variable<std::complex<float>> *
    InquireVariableCFloat(const std::string &name, const bool readIn = true);
    Variable<std::complex<double>> *
    InquireVariableCDouble(const std::string &name, const bool readIn = true);
    Variable<std::complex<long double>> *
    InquireVariableCLDouble(const std::string &name, const bool readIn = true);

    /**
     * Not implemented
     * @param name
     * @param readIn
     * @return
     */
    VariableCompound *InquireVariableCompound(const std::string &name,
                                              const bool readIn = true);

    void Close(const int transportIndex = -1);

private:
    void Init(); ///< called from constructor, gets the selected ADIOS1
                 /// transport method from settings

    template <class T>
    Variable<T> *InquireVariableCommon(const std::string &name,
                                       const bool readIn)
    {
        std::cout << "Hello ADIOS1Reader::InquireVariableCommon\n";

        // here read variable metadata (dimensions, type, etc.)...then create a
        // Variable like below:
        // Variable<T>& variable = m_ADIOS.DefineVariable<T>( m_Name + "/" +
        // name, )
        // return &variable; //return address if success
        return nullptr; // on failure
    }

    enum ADIOS_READ_METHOD read_method = ADIOS_READ_METHOD_BP;
};

} // end namespace adios

#endif /* ADIOS1READER_H_ */
