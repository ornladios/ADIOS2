/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5ReaderP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_ENGINE_HDF5_HDF5READERP_H_
#define ADIOS2_ENGINE_HDF5_HDF5READERP_H_

#include "HDF5Common.h"

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/core/Engine.h"

namespace adios
{

class HDF5Reader : public Engine
{

public:
    /**
     * Constructor for single HDF5 reader engine, reads from HDF5 format
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     */
    HDF5Reader(ADIOS &adios, const std::string name,
               const std::string accessMode, MPI_Comm mpiComm,
               const Method &method);

    virtual ~HDF5Reader();

    bool isValid();

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

    void Advance(float timeout_sec = 0.0);

    void Close(const int transportIndex = -1);

    template <typename T>
    void UseHDFRead(const std::string &variableName, T *values, hid_t h5type);

    /*
    template <class T>
    void ReadMe(Variable<T> &variable, T *values, hid_t h5type);
    */
private:
    HDF5Common _H5File;
    void Init();

    int _mpi_rank;
    int _mpi_size;
};
};
#endif /* ADIOS2_ENGINE_HDF5_HDF5READERP_H_ */
