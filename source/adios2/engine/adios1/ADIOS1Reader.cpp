/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: wfg
 */

#include "ADIOS1Reader.h"

#include "adios2/core/Support.h"
#include "adios2/core/adiosFunctions.h"           // CSVToVector
#include "adios2/transport/file/FStream.h"        // uses C++ fstream
#include "adios2/transport/file/FileDescriptor.h" // uses POSIX
#include "adios2/transport/file/FilePointer.h"    // uses C FILE*

#include <adios_error.h>

namespace adios
{

ADIOS1Reader::ADIOS1Reader(ADIOS &adios, const std::string &name,
                           const std::string accessMode, MPI_Comm mpiComm,
                           const Method &method)
: Engine(adios, "ADIOS1Reader", name, accessMode, mpiComm, method,
         " ADIOS1Reader constructor (or call to ADIOS Open).\n")
{
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

Variable<void> *
ADIOS1Reader::InquireVariable(const std::string &variableName,
                              const bool readIn) // not yet implemented
{
    return nullptr;
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

//#include "core/adios_selection_util.h"
//#include "core/util.h"
void ADIOS1Reader::ReadJoinedArray(const std::string &name, const Dims &offs,
                                   const Dims &ldims, const int fromStep,
                                   const int nSteps, void *data)
{
#if 0
    ADIOS_VARINFO *vi = adios_inq_var(m_fh, name.c_str());
    if (vi)
    {
        /* Update blockinfo: calculate start offsets now */
        adios_inq_var_blockinfo(m_fh, vi);
        int block = 0;
        int firstblock = 0; // first block in fromStep
        for (int step = 0; step < vi->nsteps; step++)
        {
            uint64_t offs = 0;
            if (step == fromStep)
                firstblock = block;
            for (int j = 0; j < vi->nblocks[step]; j++)
            {
                vi->blockinfo[block].start[0] = offs;
                offs += vi->blockinfo[block].count[0];
                ++block;
            }
        }
        ADIOS_SELECTION *bb =
            adios_selection_boundingbox(vi->ndim, offs.data(), ldims.data());
        /* Implement block-based reading here and now */
        for (int step = fromStep; step < fromStep + nSteps && step < vi->nsteps;
             step++)
        {
            /* read blocks that intersect with the selection */
            block = firstblock;
            for (int j = 0; j < vi->nblocks[step]; j++)
            {
                ADIOS_SELECTION *blockbb = adios_selection_boundingbox(
                    vi->ndim, vi->blockinfo[block].start,
                    vi->blockinfo[block].count);
                ADIOS_SELECTION *intersectbb =
                    adios_selection_intersect_global(bb, blockbb);
                if (intersectbb)
                {
                    size_t ele_num = 0;
                    for (int i = 0; i < vi->ndim; i++)
                        ele_num += vi->blockinfo[block].count[i];
                    int size_of_type = adios_type_size(vi->type, nullptr);
                    char *blockdata = malloc(ele_num * size_of_type);
                    ADIOS_SELECTION *wb = adios_selection_writeblock(j);
                    adios_schedule_read(m_fh, wb, name.c_str(), step, 1,
                                        blockdata);
                    adios_perform_reads(m_fh, 1);

                    /* Copy data into place */
                    uint64_t dst_stride;
                    uint64_t src_stride;
                    uint64_t dst_offset;
                    uint64_t src_offset;
                    std::vector<uint64_t> size_in_dset[32];

                    /* determine how many (fastest changing) dimensions can we
                     * copy in one swoop */
                    int i;
                    for (i = vi->ndim - 1; i > -1; i--)
                    {
                        if (blockbb->u.bb.start[i] == offs[i] &&
                            blockbb->u.bb.count[i] == ldims[i])
                        {
                            datasize *= ldims[i];
                        }
                        else
                            break;
                    }

                    adios_util_copy_data(data, blockdata, 0, vi->ndim,
                                         size_in_dset.data(), bbsize,
                                         ldims.data(), dst_stride, src_stride,
                                         dst_offset, src_offset, ele_num,
                                         size_of_type, adios_flag_no, vi->type);

                    adios_selection_delete(intersectbb);
                    free(blockdata);
                }
                adios_selection_delete(blockbb);
                block++;
            }
        }
        adios_selection_delete(bb);
    }
    adios_free_varinfo(vi);
#endif
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
    else if (readAsJoinedArray)
    {
        ReadJoinedArray(name, offs, ldims, fromStep, nSteps, data);
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
            adios_selection_boundingbox(ldims.size(), start, count);
        }
        adios_schedule_read(m_fh, sel, name.c_str(), (int)fromStep, (int)nSteps,
                            data);
        adios_selection_delete(sel);
    }
}

void ADIOS1Reader::ScheduleRead(Variable<char> &variable, char *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<unsigned char> &variable,
                                unsigned char *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<short> &variable, short *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<unsigned short> &variable,
                                unsigned short *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<int> &variable, int *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<unsigned int> &variable,
                                unsigned int *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<long int> &variable, long int *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<unsigned long int> &variable,
                                unsigned long int *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<long long int> &variable,
                                long long int *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<unsigned long long int> &variable,
                                unsigned long long int *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<float> &variable, float *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<double> &variable, double *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<long double> &variable,
                                long double *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<std::complex<float>> &variable,
                                std::complex<float> *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<std::complex<double>> &variable,
                                std::complex<double> *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}
void ADIOS1Reader::ScheduleRead(Variable<std::complex<long double>> &variable,
                                std::complex<long double> *values)
{
    ScheduleReadCommon(variable.m_Name, variable.m_Start, variable.m_Count,
                       variable.GetReadFromStep(), variable.GetReadNSteps(),
                       variable.ReadAsLocalValue(),
                       variable.ReadAsJoinedArray(), (void *)values);
}

void ADIOS1Reader::ScheduleRead(const std::string &variableName, char *values)
{
    ScheduleRead(m_ADIOS.GetVariable<char>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                unsigned char *values)
{
    ScheduleRead(m_ADIOS.GetVariable<unsigned char>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName, short *values)
{
    ScheduleRead(m_ADIOS.GetVariable<short>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                unsigned short *values)
{
    ScheduleRead(m_ADIOS.GetVariable<unsigned short>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName, int *values)
{
    ScheduleRead(m_ADIOS.GetVariable<int>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                unsigned int *values)
{
    ScheduleRead(m_ADIOS.GetVariable<unsigned int>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                long int *values)
{
    ScheduleRead(m_ADIOS.GetVariable<long int>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                unsigned long int *values)
{
    ScheduleRead(m_ADIOS.GetVariable<unsigned long int>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                long long int *values)
{
    ScheduleRead(m_ADIOS.GetVariable<long long int>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                unsigned long long int *values)
{
    ScheduleRead(m_ADIOS.GetVariable<unsigned long long int>(variableName),
                 values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName, float *values)
{
    ScheduleRead(m_ADIOS.GetVariable<float>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName, double *values)
{
    ScheduleRead(m_ADIOS.GetVariable<double>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                long double *values)
{
    ScheduleRead(m_ADIOS.GetVariable<long double>(variableName), values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                std::complex<float> *values)
{
    ScheduleRead(m_ADIOS.GetVariable<std::complex<float>>(variableName),
                 values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                std::complex<double> *values)
{
    ScheduleRead(m_ADIOS.GetVariable<std::complex<double>>(variableName),
                 values);
}
void ADIOS1Reader::ScheduleRead(const std::string &variableName,
                                std::complex<long double> *values)
{
    ScheduleRead(m_ADIOS.GetVariable<std::complex<long double>>(variableName),
                 values);
}

void ADIOS1Reader::PerformReads(PerformReadMode mode)
{
    adios_perform_reads(m_fh, (int)mode);
}

void ADIOS1Reader::Release() { adios_release_step(m_fh); }

void ADIOS1Reader::Advance(const float timeout_sec)
{
    Advance(AdvanceMode::NEXT_AVAILABLE, timeout_sec);
}

void ADIOS1Reader::Advance(AdvanceMode mode, const float timeout_sec)
{
    if (m_OpenAsFile)
    {
        throw std::invalid_argument("ERROR: ADIOS1Reader does not allow "
                                    "Advance() on a file which was opened for "
                                    "read as File\n");
    }
    int last = (mode == AdvanceMode::NEXT_AVAILABLE ? 0 : 1);
    float *to = const_cast<float *>(&timeout_sec);
    adios_advance_step(m_fh, last, *to);

    switch (adios_errno)
    {
    case err_no_error:
        m_AdvanceStatus = AdvanceStatus::OK;
        break;
    case err_end_of_stream:
        m_AdvanceStatus = AdvanceStatus::END_OF_STREAM;
        break;
    case err_step_notready:
        m_AdvanceStatus = AdvanceStatus::STEP_NOT_READY;
        break;
    default:
        m_AdvanceStatus = AdvanceStatus::OTHER_ERROR;
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
    if (m_DebugMode == true)
    {
        if (m_AccessMode != "r" && m_AccessMode != "read")
            throw std::invalid_argument(
                "ERROR: ADIOS1Reader doesn't support access mode " +
                m_AccessMode +
                ", in call to ADIOS Open or ADIOS1Reader constructor\n");
    }
    InitParameters();
    InitTransports();
}

void ADIOS1Reader::InitParameters()
{
    auto itOpenAsFile = m_Method.m_Parameters.find("OpenAsFile");
    if (itOpenAsFile != m_Method.m_Parameters.end())
    {
        m_OpenAsFile = true;
    }
}

void ADIOS1Reader::InitTransports()
{

    if (m_DebugMode == true)
    {
        if (TransportNamesUniqueness() == false)
        {
            throw std::invalid_argument(
                "ERROR: two transports of the same kind (e.g file IO) "
                "can't have the same name, modify with name= in Method "
                "AddTransport\n");
        }
    }

    for (const auto &parameters : m_Method.m_TransportParameters)
    {
        auto itTransport = parameters.find("transport");
        if (itTransport->second == "file" || itTransport->second == "File" ||
            itTransport->second == "bp" || itTransport->second == "BP")
        {
            m_ReadMethod = ADIOS_READ_METHOD_BP;
        }
        else
        {
            if (m_DebugMode == true)
                throw std::invalid_argument(
                    "ERROR: transport " + itTransport->second +
                    " (you mean File?) not supported, in " + m_Name +
                    m_EndMessage);
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
