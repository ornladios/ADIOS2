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

HDF5Writer::~HDF5Writer() {}

void HDF5Writer::Init()
{
    if (m_AccessMode != "w" && m_AccessMode != "write" && m_AccessMode != "a" &&
        m_AccessMode != "append")
    {
        throw std::invalid_argument(
            "ERROR: HDF5Writer doesn't support access mode " + m_AccessMode +
            ", in call to ADIOS Open or HDF5Writer constructor\n");
    }

    _H5File.H5_Init(m_Name, m_MPIComm, true);
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
    UseHDFWrite(variable, values, _H5File.DefH5T_COMPLEX_FLOAT);
}

void HDF5Writer::Write(Variable<std::complex<double>> &variable,
                       const std::complex<double> *values)
{
    UseHDFWrite(variable, values, _H5File.DefH5T_COMPLEX_DOUBLE);
}

void HDF5Writer::Write(Variable<std::complex<long double>> &variable,
                       const std::complex<long double> *values)
{
    UseHDFWrite(variable, values, _H5File.DefH5T_COMPLEX_LongDOUBLE);
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
                _H5File.DefH5T_COMPLEX_FLOAT);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<double> *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<std::complex<double>>(variableName), values,
                _H5File.DefH5T_COMPLEX_DOUBLE);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<long double> *values)
{
    UseHDFWrite(m_ADIOS.GetVariable<std::complex<long double>>(variableName),
                values, _H5File.DefH5T_COMPLEX_LongDOUBLE);
}

void HDF5Writer::Advance(float timeout_sec) { _H5File.H5_Advance(0); }

void HDF5Writer::Close(const int transportIndex)
{
    /*
    //void* hi = H5Iobject_verify(H5S_SCALAR, H5I_DATASPACE);

    hid_t s = H5Screate(H5S_SCALAR);
    //hid_t attr = H5Acreate(_H5File.m_Group_id, "NumTimeSteps",
    H5T_NATIVE_UINT,
    s, H5P_DEFAULT, H5P_DEFAULT);
    hid_t attr = H5Acreate(_H5File.m_File_id, "NumTimeSteps", H5T_NATIVE_UINT,
    s,
    H5P_DEFAULT, H5P_DEFAULT);
    uint  totalts = _H5File.m_CurrentTimeStep+1;
    H5Awrite(attr,H5T_NATIVE_UINT,&totalts);

    H5Sclose(s);
    H5Aclose(attr);
    */
    _H5File.H5_Close();
}

template <class T>
void HDF5Writer::UseHDFWrite(Variable<T> &variable, const T *values,
                             hid_t h5type)
{
    _H5File.CheckWriteGroup();
    // here comes your magic at Writing now variable.m_UserValues has the data
    // passed by the user
    // set variable
    variable.m_AppValues = values;
    m_WrittenVariables.insert(variable.m_Name);

    int dimSize = variable.m_GlobalDimensions.size();

    std::vector<hsize_t> dimsf, count, offset;

    for (int i = 0; i < dimSize; i++)
    {
        dimsf.push_back(variable.m_GlobalDimensions[i]);
        if (variable.m_LocalDimensions.size() == dimSize)
        {
            count.push_back(variable.m_LocalDimensions[i]);
            offset.push_back(variable.m_Offsets[i]);
        }
        else
        {
            count.push_back(variable.m_GlobalDimensions[i]);
            offset.push_back(0);
        }
    }

    hid_t _filespace = H5Screate_simple(dimSize, dimsf.data(), NULL);

    hid_t _dset_id =
        H5Dcreate(_H5File.m_Group_id, variable.m_Name.c_str(), h5type,
                  _filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // H5Sclose(_filespace);

    hid_t _memspace = H5Screate_simple(dimSize, count.data(), NULL);

    // Select hyperslab
    _filespace = H5Dget_space(_dset_id);
    H5Sselect_hyperslab(_filespace, H5S_SELECT_SET, offset.data(), NULL,
                        count.data(), NULL);

    //  Create property list for collective dataset write.

    hid_t _plist_id = H5Pcreate(H5P_DATASET_XFER);
#ifdef ADIOS2_HAVE_MPI
    H5Pset_dxpl_mpio(_plist_id, H5FD_MPIO_COLLECTIVE);
#endif
    herr_t status;

    status =
        H5Dwrite(_dset_id, h5type, _memspace, _filespace, _plist_id, values);

    if (status < 0)
    {
        // error
        std::cerr << " Write failed. " << std::endl;
    }

    // std::cout << " ==> User is responsible for freeing the data " <<
    // std::endl;

    H5Dclose(_dset_id);
    H5Sclose(_filespace);
    H5Sclose(_memspace);
    H5Pclose(_plist_id);
}

} // end namespace adios
