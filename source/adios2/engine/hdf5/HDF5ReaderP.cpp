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

#include "adios2/helper/adiosFunctions.h" //CSVToVector

#include <vector>

namespace adios2
{
namespace core
{
namespace engine
{

HDF5ReaderP::HDF5ReaderP(IO &io, const std::string &name, const Mode openMode,
                         helper::Comm comm)
: Engine("HDF5Reader", io, name, openMode, std::move(comm)),
  m_H5File(io.m_DebugMode)
{
    m_EndMessage = ", in call to IO HDF5Reader Open " + m_Name + "\n";
    Init();
}

HDF5ReaderP::~HDF5ReaderP()
{
    if (IsValid())
        DoClose();
}

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

    m_H5File.Init(m_Name, m_Comm, false);
    m_H5File.ParseParameters(m_IO);

    /*
    int ts = m_H5File.GetNumAdiosSteps();

    if (ts == 0)
    {
        throw std::runtime_error("This h5 file is NOT written by ADIOS2");
    }
    */
    m_H5File.ReadAttrToIO(m_IO);
    if (!m_InStreamMode)
    {
        // m_H5File.ReadVariables(0, m_IO);
        m_H5File.ReadAllVariables(m_IO);
    }
    else
    {
        // m_H5File.ReadVariables(0, m_IO);
        m_H5File.ReadAllVariables(m_IO);
    }
}

// returns slab size (>0) (datasize read in)
// returns -1 to advise do not continue
template <class T>
size_t HDF5ReaderP::ReadDataset(hid_t dataSetId, hid_t h5Type,
                                Variable<T> &variable, T *values)
{
    /*
    size_t variableStart = variable.m_StepsStart;

    if ((m_H5File.m_IsGeneratedByAdios) && (ts >= 0))
      {
              try
              {
                  m_H5File.SetAdiosStep(variableStart + ts);
              }
              catch (std::exception &e)
              {
                  printf("[Not fatal] %s\n", e.what());
                  return 0;
                  //break;
              }
          }

          std::vector<hid_t> chain;
          if (!m_H5File.OpenDataset(variable.m_Name, chain)) {
            return -1;
          }

          hid_t dataSetId = chain.back();
          interop::HDF5DatasetGuard g(chain);
          //hid_t dataSetId =
          //  H5Dopen(m_H5File.m_GroupId, variable.m_Name.c_str(), H5P_DEFAULT);
          if (dataSetId < 0)
          {
              return 0;
          }
    */
    hid_t fileSpace = H5Dget_space(dataSetId);
    interop::HDF5TypeGuard g_fs(fileSpace, interop::E_H5_SPACE);

    if (fileSpace < 0)
    {
        return 0;
    }

    size_t slabsize = 1;

    int ndims = std::max(variable.m_Shape.size(), variable.m_Count.size());
    if (0 == ndims)
    { // is scalar
        hid_t myclass = H5Tget_class(h5Type);
        if (H5Tget_class(h5Type) == H5T_STRING)
        {
            m_H5File.ReadStringScalarDataset(dataSetId, *(std::string *)values);
        }
        else
        {
            hid_t ret = H5Dread(dataSetId, h5Type, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, values);
        }
    }
    else
    {
        std::vector<hsize_t> start(ndims), count(ndims), stride(ndims);
        bool isOrderC = helper::IsRowMajor(m_IO.m_HostLanguage);

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
        hid_t ret = H5Sselect_hyperslab(fileSpace, H5S_SELECT_SET, start.data(),
                                        stride.data(), count.data(), NULL);
        if (ret < 0)
            return 0;

        hid_t memDataSpace = H5Screate_simple(ndims, count.data(), NULL);
        interop::HDF5TypeGuard g_mds(memDataSpace, interop::E_H5_SPACE);

        int elementsRead = 1;
        for (int i = 0; i < ndims; i++)
        {
            elementsRead *= count[i];
        }

        ret = H5Dread(dataSetId, h5Type, memDataSpace, fileSpace, H5P_DEFAULT,
                      values);
        // H5Sclose(memDataSpace);
    }

    return slabsize;
}

template <class T>
void HDF5ReaderP::UseHDFRead(Variable<T> &variable, T *data, hid_t h5Type)
{

    if (!m_H5File.m_IsGeneratedByAdios)
    {
        // UseHDFReadNativeFile(variable, data, h5Type);
        hid_t dataSetId =
            H5Dopen(m_H5File.m_FileId, variable.m_Name.c_str(), H5P_DEFAULT);
        if (dataSetId < 0)
        {
            return;
        }

        interop::HDF5TypeGuard g_ds(dataSetId, interop::E_H5_DATASET);
        ReadDataset(dataSetId, h5Type, variable, data);
        return;
    }

    T *values = data;
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
        try
        {
            m_H5File.SetAdiosStep(variableStart + ts);
        }
        catch (std::exception &e)
        {
            printf("[Not fatal] %s\n", e.what());
            break;
        }

        std::vector<hid_t> chain;
        if (!m_H5File.OpenDataset(variable.m_Name, chain))
        {
            return;
        }
        hid_t dataSetId = chain.back();
        interop::HDF5DatasetGuard g(chain);
        // hid_t dataSetId =
        //  H5Dopen(m_H5File.m_GroupId, variable.m_Name.c_str(), H5P_DEFAULT);
        if (dataSetId < 0)
        {
            return;
        }

        size_t slabsize = ReadDataset(dataSetId, h5Type, variable, values);

        if (slabsize == 0)
        {
            break;
        }
        // H5Sclose(fileSpace);
        // H5Dclose(dataSetId);

        ts++;
        values += slabsize;
    } // while
}

/*
 */

StepStatus HDF5ReaderP::BeginStep(StepMode mode, const float timeoutSeconds)
{
    int ts = m_H5File.GetNumAdiosSteps();

    if (m_StreamAt >= ts)
    {
        m_IO.m_ReadStreaming = false;
        return StepStatus::EndOfStream;
    }

    if (m_DeferredStack.size() > 0)
    {
        // EndStep was not called!
        return StepStatus::NotReady;
    }

    if (m_InStreamMode && (m_StreamAt == m_IO.m_EngineStep))
    {
        // EndStep() was not called after BeginStep()
        // otherwise m_StreamAt would have been increated by 1
        return StepStatus::OtherError;
    }

    m_InStreamMode = true;

    m_IO.m_ReadStreaming = true;
    m_IO.m_EngineStep = m_StreamAt;

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

#define declare_type(T)                                                        \
    for (std::string variableName : m_DeferredStack)                           \
    {                                                                          \
        const std::string type = m_IO.InquireVariableType(variableName);       \
        if (type == helper::GetType<T>())                                      \
        {                                                                      \
            Variable<T> *var = m_IO.InquireVariable<T>(variableName);          \
            if (var != nullptr)                                                \
            {                                                                  \
                if (m_InStreamMode)                                            \
                {                                                              \
                    var->m_StepsStart = m_StreamAt;                            \
                    var->m_StepsCount = 1;                                     \
                }                                                              \
                hid_t h5Type = m_H5File.GetHDF5Type<T>();                      \
                UseHDFRead(*var, var->GetData(), h5Type);                      \
            }                                                                  \
        }                                                                      \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    //
    // clear the list here so only read the variables once
    // (other functions like EndStep() will call PerformToGet() also)
    //
    m_DeferredStack.clear();
}

#define declare_type(T)                                                        \
    void HDF5ReaderP::DoGetSync(Variable<T> &variable, T *data)                \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
                                                                               \
    void HDF5ReaderP::DoGetDeferred(Variable<T> &variable, T *data)            \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
                                                                               \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    HDF5ReaderP::DoAllStepsBlocksInfo(const Variable<T> &variable) const       \
    {                                                                          \
        return GetAllStepsBlocksInfo(variable);                                \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> HDF5ReaderP::DoBlocksInfo(         \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return GetBlocksInfo(variable, step);                                  \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void HDF5ReaderP::DoClose(const int transportIndex)
{
    /*
     */
    EndStep();
    /*
     */
    m_H5File.Close();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
