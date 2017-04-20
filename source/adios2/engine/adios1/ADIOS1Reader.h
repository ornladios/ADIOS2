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

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_

#include <iostream> //this must go away

#include "adios2/ADIOSConfig.h"
#include "adios2/capsule/heap/STLVector.h"
#include "adios2/core/Engine.h"

// Fake out the include guard from ADIOS1's mpidummy.h to prevent it from
// getting included
#ifdef _NOMPI
#define __MPI_DUMMY_H__
#define MPI_Comm int
#endif
#include <adios_read_v2.h>
#ifdef _NOMPI
#undef MPI_Comm
#undef __MPI_DUMMY_H__
#endif

namespace adios
{

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

    void ScheduleRead(Variable<double> &variable, double *values);
    void ScheduleRead(const std::string variableName, double *values);

    void PerformReads(PerformReadMode mode);
    void Close(const int transportIndex = -1);

private:
    ADIOS_FILE *m_fh = nullptr; ///< ADIOS1 file handler
    void Init(); ///< called from constructor, gets the selected ADIOS1
                 /// transport method from settings
    void InitParameters();
    void InitTransports();

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

    void ScheduleReadCommon(const std::string &name, const Dims &ldims,
                            const Dims &offs, void *data);

    enum ADIOS_READ_METHOD m_ReadMethod = ADIOS_READ_METHOD_BP;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1READER_H_ */
