
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5WriterP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_ENGINE_HDF5_HDF5WRITERP_H__
#define ADIOS2_ENGINE_HDF5_HDF5WRITERP_H__

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/capsule/heap/STLVector.h"
#include "adios2/core/Engine.h"

#include <hdf5.h>

namespace adios
{

class HDF5Writer : public Engine
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
     */
    HDF5Writer(ADIOS &adios, const std::string name,
               const std::string accessMode, MPI_Comm mpiComm,
               const Method &method);

    virtual ~HDF5Writer();

    void Write(Variable<char> &variable, const char *values);
    void Write(Variable<unsigned char> &variable, const unsigned char *values);
    void Write(Variable<short> &variable, const short *values);
    void Write(Variable<unsigned short> &variable,
               const unsigned short *values);
    void Write(Variable<int> &variable, const int *values);
    void Write(Variable<unsigned int> &variable, const unsigned int *values);
    void Write(Variable<long int> &variable, const long int *values);
    void Write(Variable<unsigned long int> &variable,
               const unsigned long int *values);
    void Write(Variable<long long int> &variable, const long long int *values);
    void Write(Variable<unsigned long long int> &variable,
               const unsigned long long int *values);
    void Write(Variable<float> &variable, const float *values);
    void Write(Variable<double> &variable, const double *values);
    void Write(Variable<long double> &variable, const long double *values);
    void Write(Variable<std::complex<float>> &variable,
               const std::complex<float> *values);
    void Write(Variable<std::complex<double>> &variable,
               const std::complex<double> *values);
    void Write(Variable<std::complex<long double>> &variable,
               const std::complex<long double> *values);

    void Write(const std::string variableName, const char *values);
    void Write(const std::string variableName, const unsigned char *values);
    void Write(const std::string variableName, const short *values);
    void Write(const std::string variableName, const unsigned short *values);
    void Write(const std::string variableName, const int *values);
    void Write(const std::string variableName, const unsigned int *values);
    void Write(const std::string variableName, const long int *values);
    void Write(const std::string variableName, const unsigned long int *values);
    void Write(const std::string variableName, const long long int *values);
    void Write(const std::string variableName,
               const unsigned long long int *values);
    void Write(const std::string variableName, const float *values);
    void Write(const std::string variableName, const double *values);
    void Write(const std::string variableName, const long double *values);
    void Write(const std::string variableName,
               const std::complex<float> *values);
    void Write(const std::string variableName,
               const std::complex<double> *values);
    void Write(const std::string variableName,
               const std::complex<long double> *values);

    void Close(const int transportIndex = -1);

private:
    ///< heap capsule, contains data and metadata buffers
    capsule::STLVector m_Buffer;

    void Init();
    void clean();

    hid_t _plist_id, _file_id, _dset_id;
    hid_t _memspace, _filespace;

    hid_t DefH5T_COMPLEX_DOUBLE;
    hid_t DefH5T_COMPLEX_FLOAT;
    hid_t DefH5T_COMPLEX_LongDOUBLE;

    template <class T>
    void UseHDFWrite(Variable<T> &variable, const T *values, hid_t h5type);
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_HDF5_HDF5WRITERP_H__ */
