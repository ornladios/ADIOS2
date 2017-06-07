/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "ADIOS1Reader.h"

#include <adios_error.h>

#include "adios2/helper/adiosFunctions.h" // CSVToVector

namespace adios
{

ADIOS1Reader::ADIOS1Reader(IO &io, const std::string &name,
                           const OpenMode openMode, MPI_Comm mpiComm)
: Engine("ADIOS1Reader", io, name, openMode, mpiComm)
{
    m_EndMessage = " in call to IO Open ADIOS1Reader " + m_Name + "\n";

    Init();
    adios_read_init_method(m_ReadMethod, mpiComm, "");
    if (m_OpenAsFile)
    {
        m_fh = adios_read_open_file(name.c_str(), m_ReadMethod, mpiComm);
    }
    else
    {
        m_fh = adios_read_open(name.c_str(), m_ReadMethod, mpiComm,
                               ADIOS_LOCKMODE_CURRENT, 0.0);
    }
}

ADIOS1Reader::~ADIOS1Reader()
{
    if (m_fh != nullptr)
        adios_read_close(m_fh);
    adios_read_finalize_method(m_ReadMethod);
}

// PRIVATE
VariableBase *
ADIOS1Reader::InquireVariableUnknown(const std::string &variableName,
                                     const bool readIn)
{
    return nullptr; // not yet implemented
}

Variable<char> *
ADIOS1Reader::InquireVariableChar(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<char>(variableName, readIn);
}

Variable<unsigned char> *
ADIOS1Reader::InquireVariableUChar(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<unsigned char>(variableName, readIn);
}

Variable<short> *
ADIOS1Reader::InquireVariableShort(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<short>(variableName, readIn);
}

Variable<unsigned short> *
ADIOS1Reader::InquireVariableUShort(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<unsigned short>(variableName, readIn);
}

Variable<int> *ADIOS1Reader::InquireVariableInt(const std::string &variableName,
                                                const bool readIn)
{
    return InquireVariableCommon<int>(variableName, readIn);
}

Variable<unsigned int> *
ADIOS1Reader::InquireVariableUInt(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<unsigned int>(variableName, readIn);
}

Variable<long int> *
ADIOS1Reader::InquireVariableLInt(const std::string &variableName,
                                  const bool readIn)
{
    return InquireVariableCommon<long int>(variableName, readIn);
}

Variable<unsigned long int> *
ADIOS1Reader::InquireVariableULInt(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<unsigned long int>(variableName, readIn);
}

Variable<long long int> *
ADIOS1Reader::InquireVariableLLInt(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<long long int>(variableName, readIn);
}

Variable<unsigned long long int> *
ADIOS1Reader::InquireVariableULLInt(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<unsigned long long int>(variableName, readIn);
}

Variable<float> *
ADIOS1Reader::InquireVariableFloat(const std::string &variableName,
                                   const bool readIn)
{
    return InquireVariableCommon<float>(variableName, readIn);
}

Variable<double> *
ADIOS1Reader::InquireVariableDouble(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<double>(variableName, readIn);
}

Variable<long double> *
ADIOS1Reader::InquireVariableLDouble(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableCommon<long double>(variableName, readIn);
}

Variable<std::complex<float>> *
ADIOS1Reader::InquireVariableCFloat(const std::string &variableName,
                                    const bool readIn)
{
    return InquireVariableCommon<std::complex<float>>(variableName, readIn);
}

Variable<std::complex<double>> *
ADIOS1Reader::InquireVariableCDouble(const std::string &variableName,
                                     const bool readIn)
{
    return InquireVariableCommon<std::complex<double>>(variableName, readIn);
}

Variable<std::complex<long double>> *
ADIOS1Reader::InquireVariableCLDouble(const std::string &variableName,
                                      const bool readIn)
{
    return InquireVariableCommon<std::complex<long double>>(variableName,
                                                            readIn);
}

VariableCompound *
ADIOS1Reader::InquireVariableCompound(const std::string &variableName,
                                      const bool readIn)
{
    return nullptr;
}

void ADIOS1Reader::ScheduleReadCommon(const std::string &name, const Dims &offs,
                                      const Dims &ldims, const int fromStep,
                                      const int nSteps,
                                      const bool readAsLocalValue,
                                      const bool readAsJoinedArray, void *data)
{
    if (readAsLocalValue)
    {
        /* Get all the requested values from metadata now */
        ADIOS_VARINFO *vi = adios_inq_var(m_fh, name.c_str());
        if (vi)
        {
            adios_inq_var_stat(m_fh, vi, 0, 1);
            int elemsize = adios_type_size(vi->type, nullptr);
            long long blockidx = 0;
            for (int i = 0; i < fromStep; i++)
            {
                blockidx += vi->nblocks[i];
            }
            char *dest = (char *)data;
            for (int i = fromStep; i < fromStep + nSteps; i++)
            {
                for (int j = 0; j < vi->nblocks[i]; j++)
                {
                    memcpy(dest, vi->statistics->blocks->mins[blockidx],
                           elemsize);
                    ++blockidx;
                    dest += elemsize;
                }
            }
        }
    }
    else
    {
        uint64_t start[32], count[32];
        for (int i = 0; i < ldims.size(); i++)
        {
            start[i] = (uint64_t)offs[i];
            count[i] = (uint64_t)ldims[i];
        }
        ADIOS_SELECTION *sel = nullptr;
        if (ldims.size() > 0)
        {
            sel = adios_selection_boundingbox(ldims.size(), start, count);
        }
        adios_schedule_read(m_fh, sel, name.c_str(), (int)fromStep, (int)nSteps,
                            data);
        adios_selection_delete(sel);
    }
}

#define declare_type(T)                                                        \
    void ADIOS1Reader::DoScheduleRead(Variable<T> &variable, const T *values)  \
    {                                                                          \
        ScheduleReadCommon(variable.m_Name, variable.m_Start,                  \
                           variable.m_Count, variable.m_ReadFromStep,          \
                           variable.m_ReadNSteps, variable.m_ReadAsLocalValue, \
                           variable.m_ReadAsJoined, (void *)values);           \
    }                                                                          \
                                                                               \
    void ADIOS1Reader::DoScheduleRead(const std::string &variableName,         \
                                      const T *values)                         \
    {                                                                          \
        DoScheduleRead(m_IO.GetVariable<T>(variableName), values);             \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void ADIOS1Reader::PerformReads(ReadMode mode)
{
    adios_perform_reads(m_fh, (int)mode);
}

void ADIOS1Reader::Release() { adios_release_step(m_fh); }

void ADIOS1Reader::Advance(const float timeout_sec)
{
    Advance(AdvanceMode::NextAvailable, timeout_sec);
}

void ADIOS1Reader::Advance(AdvanceMode mode, const float timeout_sec)
{
    if (m_OpenAsFile)
    {
        throw std::invalid_argument("ERROR: ADIOS1Reader does not allow "
                                    "Advance() on a file which was opened for "
                                    "read as File\n");
    }
    int last = (mode == AdvanceMode::NextAvailable ? 0 : 1);
    float *to = const_cast<float *>(&timeout_sec);
    adios_advance_step(m_fh, last, *to);

    switch (adios_errno)
    {
    case err_no_error:
        m_AdvanceStatus = AdvanceStatus::OK;
        break;
    case err_end_of_stream:
        m_AdvanceStatus = AdvanceStatus::EndOfStream;
        break;
    case err_step_notready:
        m_AdvanceStatus = AdvanceStatus::StepNotReady;
        break;
    default:
        m_AdvanceStatus = AdvanceStatus::OtherError;
        break;
    }
}

void ADIOS1Reader::AdvanceAsync(
    AdvanceMode mode,
    std::function<void(std::shared_ptr<adios::Engine>)> callback)
{
    throw std::invalid_argument(
        "ERROR: ADIOS1Reader doesn't support AdvanceSync()\n");
}

void ADIOS1Reader::Close(const int transportIndex)
{
    adios_read_close(m_fh);
    m_fh = nullptr;
}

// PRIVATE
void ADIOS1Reader::Init()
{
    if (m_DebugMode)
    {
        if (m_OpenMode != OpenMode::Read)
        {
            throw std::invalid_argument(
                "ERROR: ADIOS1Reader only supports OpenMode::r (read) access "
                "mode " +
                m_EndMessage);
        }
    }
    InitParameters();
    InitTransports();
}

void ADIOS1Reader::InitParameters()
{
    auto itOpenAsFile = m_IO.m_Parameters.find("OpenAsFile");
    if (itOpenAsFile != m_IO.m_Parameters.end())
    {
        m_OpenAsFile = true;
    }
}

void ADIOS1Reader::InitTransports()
{

    for (const auto &parameters : m_IO.m_TransportsParameters)
    {
        auto itTransport = parameters.find("transport");
        if (itTransport->second == "file" || itTransport->second == "File" ||
            itTransport->second == "bp" || itTransport->second == "BP")
        {
            m_ReadMethod = ADIOS_READ_METHOD_BP;
        }
        else
        {
            if (m_DebugMode)
            {
                throw std::invalid_argument(
                    "ERROR: transport " + itTransport->second +
                    " (you mean File?) not supported, in " + m_Name +
                    m_EndMessage);
            }
        }
    }
}

static void CheckADIOS1Type(const std::string &name, std::string adios2Type,
                            std::string adios1Type)
{
    if (adios1Type != adios2Type)
    {
        throw std::invalid_argument(
            "Type mismatch. The expected ADIOS2 type <" + adios2Type +
            "> is not compatible with ADIOS1 type <" + adios1Type +
            "> of the requested variable '" + name + "'\n");
    }
}

bool ADIOS1Reader::CheckADIOS1TypeCompatibility(const std::string &name,
                                                std::string adios2Type,
                                                enum ADIOS_DATATYPES adios1Type)
{
    bool compatible = false;
    switch (adios1Type)
    {
    case adios_unsigned_byte:
        CheckADIOS1Type(name, adios2Type, "unsigned char");
        break;
    case adios_unsigned_short:
        CheckADIOS1Type(name, adios2Type, "unsigned short");
        break;
    case adios_unsigned_integer:
        CheckADIOS1Type(name, adios2Type, "unsigned int");
        break;
    case adios_unsigned_long:
        CheckADIOS1Type(name, adios2Type, "unsigned long long int");
        break;

    case adios_byte:
        CheckADIOS1Type(name, adios2Type, "char");
        break;
    case adios_short:
        CheckADIOS1Type(name, adios2Type, "short");
        break;
    case adios_integer:
        CheckADIOS1Type(name, adios2Type, "int");
        break;
    case adios_long:
        CheckADIOS1Type(name, adios2Type, "long long int");
        break;

    case adios_real:
        CheckADIOS1Type(name, adios2Type, "float");
        break;
    case adios_double:
        CheckADIOS1Type(name, adios2Type, "double");
        break;
    case adios_long_double:
        CheckADIOS1Type(name, adios2Type, "long double");
        break;

    case adios_string:
        CheckADIOS1Type(name, adios2Type, "string");
        break;
    case adios_complex:
        CheckADIOS1Type(name, adios2Type, "float complex");
        break;
    case adios_double_complex:
        CheckADIOS1Type(name, adios2Type, "double complex");
        break;

    case adios_string_array:
        compatible = false;
        break;
    default:
        compatible = false;
    }
    return true;
}

} // end namespace
