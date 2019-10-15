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

#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{
namespace core
{
namespace engine
{

HDF5WriterP::HDF5WriterP(IO &io, const std::string &name, const Mode mode,
                         helper::Comm comm)
: Engine("HDF5Writer", io, name, mode, std::move(comm)),
  m_H5File(io.m_DebugMode)
{
    m_IO.m_ReadStreaming = false;
    m_EndMessage = ", in call to IO HDF5Writer Open " + m_Name + "\n";
    Init();
}

HDF5WriterP::~HDF5WriterP() { DoClose(); }

StepStatus HDF5WriterP::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_IO.m_ReadStreaming = false;
    return StepStatus::OK;
}

void HDF5WriterP::EndStep()
{
    m_H5File.Advance();
    m_H5File.WriteAttrFromIO(m_IO);
}

void HDF5WriterP::PerformPuts() {}

// PRIVATE
void HDF5WriterP::Init()
{
    if (m_OpenMode != Mode::Write && m_OpenMode != Mode::Append)
    {
        throw std::invalid_argument(
            "ERROR: HDF5Writer only support OpenMode::Write or "
            "OpenMode::Append "
            ", in call to ADIOS Open or HDF5Writer constructor\n");
    }

#ifdef NEVER
    m_H5File.Init(m_Name, m_Comm, true);
#else
    // enforce .h5 ending
    std::string suffix = ".h5";
    std::string wrongSuffix = ".bp";

    int ss = m_Name.size();
    int wpos = m_Name.find(wrongSuffix);

    if (wpos == ss - wrongSuffix.size())
    {
        // is a file with .bp ending
        std::string updatedName = m_Name.substr(0, wpos) + suffix;
        m_H5File.Init(updatedName, m_Comm, true);
    }
    else
    {
        m_H5File.Init(m_Name, m_Comm, true);
    }
    m_H5File.ParseParameters(m_IO);
#endif
}

#define declare_type(T)                                                        \
    void HDF5WriterP::DoPutSync(Variable<T> &variable, const T *values)        \
    {                                                                          \
        DoPutSyncCommon(variable, values);                                     \
    }                                                                          \
    void HDF5WriterP::DoPutDeferred(Variable<T> &variable, const T *values)    \
    {                                                                          \
        DoPutSyncCommon(variable, values);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

template <class T>
void HDF5WriterP::DoPutSyncCommon(Variable<T> &variable, const T *values)
{

    bool isOrderC = helper::IsRowMajor(m_IO.m_HostLanguage);

    if (!isOrderC)
    {
        int ndims = std::max(variable.m_Shape.size(), variable.m_Count.size());

        if (ndims > 1)
        {
            Dims c_shape(ndims), c_offset(ndims), c_count(ndims);
            for (int i = 0; i < ndims; i++)
            {
                c_shape[i] = variable.m_Shape[ndims - i - 1];
                c_offset[i] = variable.m_Start[ndims - i - 1];
                c_count[i] = variable.m_Count[ndims - i - 1];
            }

            Variable<T> dup =
                Variable<T>(variable.m_Name, c_shape, c_offset, c_count,
                            variable.IsConstantDims(), false);

            /*
             * duplicate var attributes and convert to c order before saving.
             */
            dup.SetData(values);
            m_H5File.Write(dup, values);
            return;
        }
    }
    variable.SetData(values);
    m_H5File.Write(variable, values);
}

// I forced attribute writing to hdf5 in Endstep().
// So Do not call engine.Flush()
// unless you are using ascent
// Flush() call makes DoClose skips m_H5File.Close() so ascent calls sequences
// work.

void HDF5WriterP::Flush(const int transportIndex)
{
    m_H5File.WriteAttrFromIO(m_IO);

    m_Flushed = true;
}

void HDF5WriterP::DoClose(const int transportIndex)
{
    if (!m_Flushed)
    {
        m_H5File.WriteAttrFromIO(m_IO);
        m_H5File.Close();
    }
    else
    {
        // printf("flushed, no close (##asend usage) \n");
        // m_H5File.Close();
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
