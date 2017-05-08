/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5WriterP.cpp
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include "HDF5WriterP.h"

#include "adios2/ADIOSMPI.h"
#include "adios2/core/Support.h"
#include "adios2/core/adiosFunctions.h" //CSVToVector

namespace adios
{

HDF5Writer::HDF5Writer(ADIOS &adios, const std::string name,
                       const std::string accessMode, MPI_Comm mpiComm,
                       const Method &method)
: Engine(adios, "HDF5Writer", name, accessMode, mpiComm,
         method, /*debugMode, cores,*/
         " HDF5Writer constructor (or call to ADIOS Open).\n"),
  m_Buffer(m_DebugMode)
{
    Init();
}

HDF5Writer::~HDF5Writer() { Close(); }

void HDF5Writer::Init()
{
    if (m_AccessMode != "w" && m_AccessMode != "write" && m_AccessMode != "a" &&
        m_AccessMode != "append")
    {
        throw std::invalid_argument(
            "ERROR: HDF5Writer doesn't support access mode " + m_AccessMode +
            ", in call to ADIOS Open or HDF5Writer constructor\n");
    }

    m_H5File.Init(m_Name, m_MPIComm, true);
}

void HDF5Writer::Write(Variable<char> &variable, const char *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_CHAR);
}

void HDF5Writer::Write(Variable<unsigned char> &variable,
                       const unsigned char *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_UCHAR);
}

void HDF5Writer::Write(Variable<short> &variable, const short *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_SHORT);
}

void HDF5Writer::Write(Variable<unsigned short> &variable,
                       const unsigned short *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_USHORT);
}

void HDF5Writer::Write(Variable<int> &variable, const int *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_INT);
}

void HDF5Writer::Write(Variable<unsigned int> &variable,
                       const unsigned int *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_UINT);
}

void HDF5Writer::Write(Variable<long int> &variable, const long int *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_LONG);
}

void HDF5Writer::Write(Variable<unsigned long int> &variable,
                       const unsigned long int *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_ULONG);
}

void HDF5Writer::Write(Variable<long long int> &variable,
                       const long long int *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_LLONG);
}

void HDF5Writer::Write(Variable<unsigned long long int> &variable,
                       const unsigned long long int *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_ULLONG);
}

void HDF5Writer::Write(Variable<float> &variable, const float *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_FLOAT);
}

void HDF5Writer::Write(Variable<double> &variable, const double *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_DOUBLE);
}

void HDF5Writer::Write(Variable<long double> &variable,
                       const long double *values)
{
    UseHDFWrite(variable, values, H5T_NATIVE_LDOUBLE);
}

void HDF5Writer::Write(Variable<std::complex<float>> &variable,
                       const std::complex<float> *values)
{
    UseHDFWrite(variable, values, m_H5File.m_DefH5TypeComplexFloat);
}

void HDF5Writer::Write(Variable<std::complex<double>> &variable,
                       const std::complex<double> *values)
{
    UseHDFWrite(variable, values, m_H5File.m_DefH5TypeComplexDouble);
}

void HDF5Writer::Write(Variable<std::complex<long double>> &variable,
                       const std::complex<long double> *values)
{
    UseHDFWrite(variable, values, m_H5File.m_DefH5TypeComplexLongDouble);
}

// String version
void HDF5Writer::Write(const std::string variableName, const char *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<char>(variableName), values,
                H5T_NATIVE_CHAR);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned char *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<unsigned char>(variableName), values,
                H5T_NATIVE_UCHAR);
}

void HDF5Writer::Write(const std::string variableName, const short *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<short>(variableName), values,
                H5T_NATIVE_SHORT);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned short *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<unsigned short>(variableName), values,
                H5T_NATIVE_USHORT);
}

void HDF5Writer::Write(const std::string variableName, const int *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<int>(variableName), values, H5T_NATIVE_INT);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned int *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<unsigned int>(variableName), values,
                H5T_NATIVE_UINT);
}

void HDF5Writer::Write(const std::string variableName, const long int *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<long int>(variableName), values,
                H5T_NATIVE_LONG);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned long int *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<unsigned long int>(variableName), values,
                H5T_NATIVE_ULONG);
}

void HDF5Writer::Write(const std::string variableName,
                       const long long int *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<long long int>(variableName), values,
                H5T_NATIVE_LLONG);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned long long int *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<unsigned long long int>(variableName),
                values, H5T_NATIVE_ULLONG);
}

void HDF5Writer::Write(const std::string variableName, const float *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<float>(variableName), values,
                H5T_NATIVE_FLOAT);
}

void HDF5Writer::Write(const std::string variableName, const double *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<double>(variableName), values,
                H5T_NATIVE_DOUBLE);
}

void HDF5Writer::Write(const std::string variableName,
                       const long double *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<long double>(variableName), values,
                H5T_NATIVE_LDOUBLE);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<float> *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<std::complex<float>>(variableName), values,
                m_H5File.m_DefH5TypeComplexFloat);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<double> *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<std::complex<double>>(variableName), values,
                m_H5File.m_DefH5TypeComplexDouble);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<long double> *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<std::complex<long double>>(variableName),
                values, m_H5File.m_DefH5TypeComplexLongDouble);
}

void HDF5Writer::Advance(float timeoutSec) { m_H5File.Advance(); }

void HDF5Writer::Close(const int transportIndex) { m_H5File.Close(); }

template <class T>
void HDF5Writer::UseHDFWrite(Variable<T> &variable, const T *values,
                             hid_t h5Type)
{
    m_H5File.CheckWriteGroup();
    // here comes your magic at Writing now variable.m_UserValues has the data
    // passed by the user
    // set variable
    variable.m_AppValues = values;
    m_WrittenVariables.insert(variable.m_Name);

    int dimSize = std::max(variable.m_Shape.size(),
                           variable.m_Count.size());

    std::vector<hsize_t> dimsf, count, offset;

    for (int i = 0; i < dimSize; i++)
    {
        if (variable.m_Shape.size() == dimSize)
        {
            dimsf.push_back(variable.m_Shape[i]);
        }
        else
        {
            dimsf.push_back(variable.m_Count[i]);
        }

        if (variable.m_Count.size() == dimSize)
        {
            count.push_back(variable.m_Count[i]);
            if (variable.m_Start.size() == dimSize)
            {
                offset.push_back(variable.m_Start[i]);
            }
            else
            {
                offset.push_back(0);
            }
        }
        else
        {
            count.push_back(variable.m_Shape[i]);
            offset.push_back(0);
        }

    }

    hid_t fileSpace = H5Screate_simple(dimSize, dimsf.data(), NULL);

    hid_t dsetID =
        H5Dcreate(m_H5File.m_GroupId, variable.m_Name.c_str(), h5Type,
                  fileSpace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // H5Sclose(fileSpace);

    hid_t memSpace = H5Screate_simple(dimSize, count.data(), NULL);

    // Select hyperslab
    fileSpace = H5Dget_space(dsetID);
    H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, offset.data(), NULL,
                        count.data(), NULL);

    //  Create property list for collective dataset write.

    hid_t plistID = H5Pcreate(H5P_DATASET_XFER);
#ifdef ADIOS2_HAVE_MPI
    H5Pset_dxpl_mpio(plistID, H5FD_MPIO_COLLECTIVE);
#endif
    herr_t status;

    status = H5Dwrite(dsetID, h5Type, memSpace, fileSpace, plistID, values);

    if (status < 0)
    {
        // error
        std::cerr << " Write failed. " << std::endl;
    }

    H5Dclose(dsetID);
    H5Sclose(fileSpace);
    H5Sclose(memSpace);
    H5Pclose(plistID);
}

} // end namespace adios
