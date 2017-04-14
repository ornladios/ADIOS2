
/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5WriterP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef HDF5_WRITER_P_H_
#define HDF5_WRITER_P_H_

#include <unistd.h> //sleep must be removed

#include "core/Engine.h"


// supported capsules
#include "capsule/heap/STLVector.h"

#include "ADIOS_MPI.h"

#include <hdf5.h>

namespace adios {
  /*
typedef struct {
  double _re; //real part
  double _im; //imaginary part
} ADIOS2_Complex_Double;

typedef struct {
  float _re; // real
  float _im; // imaginary part
} ADIOS2_Complex_Float;

// using ADIOS2_Complex_Float = std::complex<float>;

typedef struct {
  long double _re; //real part
  long double _im; //imaginary part
} ADIOS2_Complex_LongDouble;
*/
class HDF5Writer : public Engine {

public:
  /**
   * Constructor for single BP capsule engine, writes in BP format into a single
   * heap capsule
   * @param name unique name given to the engine
   * @param accessMode
   * @param mpiComm
   * @param method
   * @param debugMode
   */
  HDF5Writer(ADIOS &adios, const std::string name, const std::string accessMode,
             MPI_Comm mpiComm, const Method &method);

  virtual ~HDF5Writer();

  void Write(Variable<char> &variable, const char *values);
  void Write(Variable<unsigned char> &variable, const unsigned char *values);
  void Write(Variable<short> &variable, const short *values);
  void Write(Variable<unsigned short> &variable, const unsigned short *values);
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
  void Write(const std::string variableName, const std::complex<float> *values);
  void Write(const std::string variableName,
             const std::complex<double> *values);
  void Write(const std::string variableName,
             const std::complex<long double> *values);

  void Close(const int transportIndex = -1);
  /*
  {
      std::cout<<" ===> CLOSING HDF5 <===== "<<std::endl;
      //H5Dclose(_dset_id);
      //H5Sclose(_filespace);
      //H5Sclose(_memspace);
      //H5Pclose(_plist_id);
      H5Fclose(_file_id);
  }
  */
private:
  capsule::STLVector
      m_Buffer; ///< heap capsule, contains data and metadata buffers

  void Init();
  void clean();

  hid_t _plist_id, _file_id, _dset_id;
  hid_t _memspace, _filespace;

  hid_t DefH5T_COMPLEX_DOUBLE;
  hid_t DefH5T_COMPLEX_FLOAT;
  hid_t DefH5T_COMPLEX_LongDOUBLE;

  //hid_t DefH5T_filetype_COMPLEX_DOUBLE;
  //hid_t DefH5T_filetype_COMPLEX_FLOAT;
  //hid_t DefH5T_filetype_COMPLEX_LongDOUBLE;

  template <class T>
  void UseHDFWrite(Variable<T> &variable, const T *values, hid_t h5type);

  /*
  template<class T>
    void UseHDFWrite( Variable<T>& variable, const T* values, hid_t h5type )
  {
      //here comes your magic at Writing now variable.m_UserValues has the data
  passed by the user
      //set variable
      variable.m_AppValues = values;
      m_WrittenVariables.insert( variable.m_Name );

      std::cout<<"writting : "<<variable.m_Name<<" dim
  size:"<<variable.m_GlobalDimensions.size()<<std::endl;

      int dimSize = variable.m_GlobalDimensions.size();
      //hsize_t     dimsf[dimSize], count[dimSize], offset[dimSize];
      std::vector<hsize_t>  dimsf, count, offset;

      for(int i=0; i<dimSize; i++) {
        dimsf.push_back(variable.m_GlobalDimensions[i]);
        count.push_back(dimsf[i]);
        offset.push_back(0);
      }
      count[0] = dimsf[0]/m_SizeMPI;
      offset[0]= m_RankMPI*count[0];

      _filespace = H5Screate_simple(dimSize, dimsf.data(), NULL);

      _dset_id = H5Dcreate(_file_id, variable.m_Name.c_str(), h5type,
  _filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose(_filespace);

      _memspace = H5Screate_simple(dimSize, count.data(), NULL);

      //Select hyperslab

      _filespace = H5Dget_space(_dset_id);
      H5Sselect_hyperslab(_filespace, H5S_SELECT_SET, offset.data(), NULL,
  count.data(), NULL);


      //  Create property list for collective dataset write.

      _plist_id = H5Pcreate(H5P_DATASET_XFER);
      H5Pset_dxpl_mpio(_plist_id, H5FD_MPIO_COLLECTIVE);

      herr_t status;
      if (m_SizeMPI == 1) {
        status = H5Dwrite(_dset_id, h5type, _memspace, _filespace, _plist_id,
  values);
      } else {
        std::vector<T> val;
        for (int i=0; i<100; i++) {
          //hsize_t c = count[i];
          //hsize_t o = offset[i];
          val.push_back(0);
        }
        status = H5Dwrite(_dset_id, h5type, _memspace, _filespace, _plist_id,
  val.data());
      }

      if (status < 0) {
        // error
        std::cerr<<" Write failed. "<<std::endl;
      }

      std::cout <<" ==> User is responsible for freeing the data "<<std::endl;
      //free(values);


      //Close/release resources.

      //H5Dclose(_dset_id);
      //H5Sclose(_filespace);
      //H5Sclose(_memspace);
      //H5Pclose(_plist_id);
      //H5Fclose(_file_id);

      H5Dclose(_dset_id);
      H5Sclose(_filespace);
      H5Sclose(_memspace);
      H5Pclose(_plist_id);

  }
  */
};

} // end namespace adios

#endif /* HDF5_WRITER_P_H_ */
