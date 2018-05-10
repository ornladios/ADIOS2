/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5ReaderP.cpp
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#include "HDF5ReaderP.h"
#include "HDF5ReaderP.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

HDF5ReaderP::HDF5ReaderP(IO &io, const std::string &name, const Mode openMode,
                         MPI_Comm mpiComm)
: Engine("HDF5Reader", io, name, openMode, mpiComm), m_H5File(io.m_DebugMode)
{
    m_EndMessage = ", in call to IO HDF5Reader Open " + m_Name + "\n";
    Init();
}

HDF5ReaderP::~HDF5ReaderP() { DoClose(); }

bool HDF5ReaderP::IsValid()
{
    bool isValid = false;

    if (m_OpenMode != Mode::Read)
    {
        return isValid;
    }
    if (m_H5File.m_FileId >= 0)
    {
        isValid = true;
    }
    return isValid;
}

void HDF5ReaderP::Init()
{
    if (m_OpenMode != Mode::Read)
    {
        throw std::invalid_argument(
            "ERROR: HDF5Reader only supports OpenMode::Read "
            ", in call to Open\n");
    }

    m_H5File.Init(m_Name, m_MPIComm, false);

    /*
    int ts = m_H5File.GetNumAdiosSteps();

    if (ts == 0)
    {
        throw std::runtime_error("This h5 file is NOT written by ADIOS2");
    }
    */
    if (!m_InStreamMode)
    {
        m_H5File.ReadAllVariables(m_IO);
    }
    else
    {
        m_H5File.ReadAllVariables(m_IO);
    }
}

template <class T>
void HDF5ReaderP::UseHDFRead(Variable<T> &variable, T *data, hid_t h5Type)
{

    T *values = data;

    if (!m_H5File.m_IsGeneratedByAdios)
    {
        // printf("Will read by Native reader ..%s\n", variable.m_Name.c_str());
        hid_t dataSetId =
            H5Dopen(m_H5File.m_FileId, variable.m_Name.c_str(), H5P_DEFAULT);
        if (dataSetId < 0)
        {
            return;
        }

        hid_t fileSpace = H5Dget_space(dataSetId);
        if (fileSpace < 0)
        {
            return;
        }

        size_t slabsize = 1;

        int ndims = std::max(variable.m_Shape.size(), variable.m_Count.size());
        if (0 == ndims)
        { // is scalar
            hid_t ret = H5Dread(dataSetId, h5Type, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, values);
        }
        else
        {
            hsize_t start[ndims], count[ndims], stride[ndims];

            bool isOrderC = IsRowMajor(m_IO.m_HostLanguage);
            for (int i = 0; i < ndims; i++)
            {
                if (isOrderC)
                {
                    count[i] = variable.m_Count[i];
                    start[i] = variable.m_Start[i];
                }
                else
                {
                    count[i] = variable.m_Count[ndims - 1 - i];
                    start[i] = variable.m_Start[ndims - 1 - i];
                }

                slabsize *= count[i];
                stride[i] = 1;
            }
            hid_t ret = H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start,
                                            stride, count, NULL);
            if (ret < 0)
                return;

            hid_t memDataSpace = H5Screate_simple(ndims, count, NULL);
            int elementsRead = 1;
            for (int i = 0; i < ndims; i++)
            {
                elementsRead *= count[i];
            }

            ret = H5Dread(dataSetId, h5Type, memDataSpace, fileSpace,
                          H5P_DEFAULT, values);
            H5Sclose(memDataSpace);
        }

        H5Sclose(fileSpace);
        H5Dclose(dataSetId);

        return;
    }

    int ts = 0;
    // T *values = data;
    size_t variableStart = variable.m_StepsStart;
    /*
      // looks like m_StepsStart is defaulted to be 0 now.
    if (!m_InStreamMode && (variableStart == 1))
    { // variableBase::m_StepsStart min=1
        variableStart = 0;
    }
    */

    while (ts < variable.m_StepsCount)
    {
        if (m_H5File.m_IsGeneratedByAdios)
        {
            m_H5File.SetAdiosStep(variableStart + ts);
        }
        hid_t dataSetId =
            H5Dopen(m_H5File.m_GroupId, variable.m_Name.c_str(), H5P_DEFAULT);
        if (dataSetId < 0)
        {
            return;
        }

        hid_t fileSpace = H5Dget_space(dataSetId);
        if (fileSpace < 0)
        {
            return;
        }

        size_t slabsize = 1;

        int ndims = std::max(variable.m_Shape.size(), variable.m_Count.size());
        if (0 == ndims)
        { // is scalar
            hid_t ret = H5Dread(dataSetId, h5Type, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, values);
        }
        else
        {
            hsize_t start[ndims], count[ndims], stride[ndims];
            bool isOrderC = IsRowMajor(m_IO.m_HostLanguage);

            for (int i = 0; i < ndims; i++)
            {
                if (isOrderC)
                {
                    count[i] = variable.m_Count[i];
                    start[i] = variable.m_Start[i];
                }
                else
                {
                    count[i] = variable.m_Count[ndims - 1 - i];
                    start[i] = variable.m_Start[ndims - 1 - i];
                }
                slabsize *= count[i];
                stride[i] = 1;
            }
            hid_t ret = H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start,
                                            stride, count, NULL);
            if (ret < 0)
                return;

            hid_t memDataSpace = H5Screate_simple(ndims, count, NULL);
            int elementsRead = 1;
            for (int i = 0; i < ndims; i++)
            {
                elementsRead *= count[i];
            }

            ret = H5Dread(dataSetId, h5Type, memDataSpace, fileSpace,
                          H5P_DEFAULT, values);
            H5Sclose(memDataSpace);
        }

        H5Sclose(fileSpace);
        H5Dclose(dataSetId);

        ts++;
        values += slabsize;
    } // while
}

/*
template <class T>
void HDF5ReaderP::UseHDFRead(const std::string &variableName, T *values,
                           hid_t h5Type)
{
  int rank, size;
  MPI_Comm_rank(m_MPIComm, &rank);
  MPI_Comm_size(m_MPIComm, &size);

  hid_t dataSetId =
      H5Dopen(m_H5File.m_GroupId, variableName.c_str(), H5P_DEFAULT);

  if (dataSetId < 0)
  {
      return;
  }

  hid_t fileSpace = H5Dget_space(dataSetId);

  if (fileSpace < 0)
  {
      return;
  }
  int ndims = H5Sget_simple_extent_ndims(fileSpace);

  if (ndims == 0) { // SCALAR
    hid_t ret = H5Dread(dataSetId, h5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT,
values); return;
  }

  hsize_t dims[ndims];
  herr_t status_n = H5Sget_simple_extent_dims(fileSpace, dims, NULL);

  // hsize_t start[ndims] = {0}, count[ndims] = {0}, stride[ndims] = {1};
  hsize_t start[ndims], count[ndims], stride[ndims];

  int totalElements = 1;
  for (int i = 0; i < ndims; i++)
  {
      count[i] = dims[i];
      totalElements *= dims[i];
      start[i] = 0;
      //count[i] = 0;
      stride[i] = 1;
  }

  start[0] = rank * dims[0] / size;
  count[0] = dims[0] / size;
  if (rank == size - 1)
  {
      count[0] = dims[0] - count[0] * (size - 1);
  }

  hid_t ret = H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start, stride,
                                  count, NULL);
  if (ret < 0)
  {
      return;
  }

  hid_t memDataSpace = H5Screate_simple(ndims, count, NULL);

  int elementsRead = 1;
  for (int i = 0; i < ndims; i++)
  {
      elementsRead *= count[i];
  }

#ifdef NEVER
  //T data_array[elementsRead];

  std::vector<T> data_vector;
  data_vector.reserve(elementsRead);
  T* data_array = data_vector.data();
  ret = H5Dread(dataSetId, h5Type, memDataSpace, fileSpace, H5P_DEFAULT,
                data_array);
#else
  ret = H5Dread(dataSetId, h5Type, memDataSpace, fileSpace, H5P_DEFAULT,
values); #endif


  H5Sclose(memDataSpace);

  H5Sclose(fileSpace);
  H5Dclose(dataSetId);
}
*/

StepStatus HDF5ReaderP::BeginStep(StepMode mode, const float timeoutSeconds)
{
    // printf(".... in begin step: \n");
    m_InStreamMode = true;
    int ts = m_H5File.GetNumAdiosSteps();
    if (m_StreamAt >= ts)
    {
        return StepStatus::EndOfStream;
    }

    return StepStatus::OK;
}

size_t HDF5ReaderP::CurrentStep() const { return m_StreamAt; }

void HDF5ReaderP::EndStep()
{
    if (m_DeferredStack.size() > 0)
    {
        PerformGets();
    }
    m_StreamAt++;
    m_H5File.Advance();
}

void HDF5ReaderP::PerformGets()
{
    // looks this this is not enforced to be specific to stream mode!!
    if (!m_InStreamMode)
    {
#define declare_type(T)							\
      for (std::string variableName : m_DeferredStack)			\
	{								\
	  Variable<T> *var = m_IO.InquireVariable<T>(variableName);	\
	  if (var != nullptr)						\
	    {								\
	      hid_t h5Type = m_H5File.GetHDF5Type<T>();			\
	      UseHDFRead(*var, var->GetData(), h5Type);			\
	      break;							\
	    }								\
	}
      ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

        //throw std::runtime_error(
	//  "PerformGets() needs to follow stream read sequeuences.");
      return;
    }
#define declare_type(T)                                                        \
    for (std::string variableName : m_DeferredStack)                           \
    {                                                                          \
        Variable<T> *var = m_IO.InquireVariable<T>(variableName);              \
        if (var != nullptr)                                                    \
        {                                                                      \
            var->m_StepsStart = m_StreamAt;                                    \
            var->m_StepsCount = 1;                                             \
            hid_t h5Type = m_H5File.GetHDF5Type<T>();                          \
            UseHDFRead(*var, var->GetData(), h5Type);                          \
            break;                                                             \
        }                                                                      \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    m_DeferredStack.clear();
}

#define declare_type(T)                                                        \
    void HDF5ReaderP::DoGetSync(Variable<T> &variable, T *data)                \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void HDF5ReaderP::DoGetDeferred(Variable<T> &variable, T *data)            \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    void HDF5ReaderP::DoGetDeferred(Variable<T> &variable, T &data)            \
    {                                                                          \
        GetDeferredCommon(variable, &data);                                    \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void HDF5ReaderP::DoClose(const int transportIndex)
{
    // printf("ReaderP::DoClose() %lu\n", m_DeferredStack.size());
    if (m_DeferredStack.size() > 0)
    {
        if (m_InStreamMode)
        {
            PerformGets();
        }
        else
        {
#define declare_type(T)                                                        \
    for (std::string variableName : m_DeferredStack)                           \
    {                                                                          \
        Variable<T> *var = m_IO.InquireVariable<T>(variableName);              \
        if (var != nullptr)                                                    \
        {                                                                      \
            hid_t h5Type = m_H5File.GetHDF5Type<T>();                          \
            UseHDFRead(*var, var->GetData(), h5Type);                          \
            break;                                                             \
        }                                                                      \
    }
            ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
            m_DeferredStack.clear();
        }
    }

    m_H5File.Close();
}

} // end namespace adios2
