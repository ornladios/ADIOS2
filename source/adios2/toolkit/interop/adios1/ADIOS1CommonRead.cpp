/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1CommonRead.cpp
 *
 *  Created on: Oct 26, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *              William F Godoy godoywf@ornl.gov
 */

#include "ADIOS1CommonRead.h"
#include "ADIOS1CommonRead.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //OpenModeToString, GetType

extern int adios_verbose_level;
extern int adios_errno;

namespace adios2
{
namespace interop
{

ADIOS1CommonRead::ADIOS1CommonRead(const std::string &fileName,
                                   MPI_Comm mpiComm, const bool debugMode)
: ADIOS1Common(fileName, mpiComm, debugMode)
{
    Init();
}

ADIOS1CommonRead::~ADIOS1CommonRead()
{
    Close();
    adios_read_finalize_method(m_ReadMethod);
}

bool ADIOS1CommonRead::Open(IO &io)
{
    if (m_OpenAsFile)
    {
        m_fh =
            adios_read_open_file(m_FileName.c_str(), m_ReadMethod, m_MPIComm);
        GenerateVariables(io);
        GenerateAttributes(io);
    }
    else
    {
        m_fh = adios_read_open(m_FileName.c_str(), m_ReadMethod, m_MPIComm,
                               ADIOS_LOCKMODE_CURRENT, 0.0);
    }
    return (m_fh != NULL);
}

void ADIOS1CommonRead::DefineADIOS2Variable(IO &io, const char *name,
                                            const ADIOS_VARINFO *vi, Dims gdims,
                                            bool isJoined, bool isGlobal)
{
    switch (vi->type)
    {
    case adios_unsigned_byte:
        DefineADIOS2Variable<unsigned char>(io, name, vi, gdims, isJoined,
                                            isGlobal);
        break;
    case adios_unsigned_short:
        DefineADIOS2Variable<unsigned short>(io, name, vi, gdims, isJoined,
                                             isGlobal);
        break;
    case adios_unsigned_integer:
        DefineADIOS2Variable<unsigned int>(io, name, vi, gdims, isJoined,
                                           isGlobal);
        break;
    case adios_unsigned_long:
        DefineADIOS2Variable<uint64_t>(io, name, vi, gdims, isJoined, isGlobal);
        break;

    case adios_byte:
        DefineADIOS2Variable<signed char>(io, name, vi, gdims, isJoined,
                                          isGlobal);
        break;
    case adios_short:
        DefineADIOS2Variable<short>(io, name, vi, gdims, isJoined, isGlobal);
        break;
    case adios_integer:
        DefineADIOS2Variable<int>(io, name, vi, gdims, isJoined, isGlobal);
        break;
    case adios_long:
        DefineADIOS2Variable<int64_t>(io, name, vi, gdims, isJoined, isGlobal);
        break;

    case adios_real:
        DefineADIOS2Variable<float>(io, name, vi, gdims, isJoined, isGlobal);
        break;
    case adios_double:
        DefineADIOS2Variable<double>(io, name, vi, gdims, isJoined, isGlobal);
        break;
    case adios_long_double:
        DefineADIOS2Variable<long double>(io, name, vi, gdims, isJoined,
                                          isGlobal);
        break;

    case adios_string:
        /*FIXME: DefineADIOS2Variable<std::string>(io, name, vi, gdims,
           isJoined,
                                          isGlobal);*/
        break;
    case adios_complex:
        DefineADIOS2Variable<std::complex<float>>(io, name, vi, gdims, isJoined,
                                                  isGlobal);
        break;
    case adios_double_complex:
        DefineADIOS2Variable<std::complex<double>>(io, name, vi, gdims,
                                                   isJoined, isGlobal);
        break;
    default:
        break;
    }
}

void ADIOS1CommonRead::GenerateVariables(IO &io)
{
    if (!m_fh)
        return;
    /* Create a Variable for each variable in the file */
    for (int varid = 0; varid < m_fh->nvars; varid++)
    {
        // here read variable metadata (dimensions, type, etc.)...then create a
        // Variable like below:
        // Variable<T>& variable = io.DefineVariable<T>( m_Name + "/" +
        // name, )
        // return &variable; //return address if success
        ADIOS_VARINFO *vi = adios_inq_var(m_fh, m_fh->var_namelist[varid]);
        if (vi != nullptr)
        {
            if (vi->ndim > 0)
            {
                Dims gdims = Uint64ArrayToSizetVector(vi->ndim, vi->dims);

                bool joinedread = false;
                if (gdims[0] == JoinedDim)
                {
                    /* Joined Array */
                    InqVarBlockInfo(vi);
                    size_t joined_size = 0;
                    for (int i = 0; i < *vi->nblocks; i++)
                    {
                        joined_size += vi->blockinfo[i].count[0];
                    }
                    gdims[0] = joined_size;
                    joinedread = true;
                }

                if (!vi->global)
                {
                    /* Local array */
                    for (int j = 0; j < vi->ndim; ++j)
                    {
                        gdims[j] = IrregularDim;
                    }
                }
                else
                {
                    /* Check if dimensions change in time */
                    for (int step = 1; step < vi->nsteps; ++step)
                    {
                        Dims dims =
                            gdims; // GetGlobalDimsAtStep(vi, step, joinedread);
                        for (int j = 0; j < vi->ndim; ++j)
                        {
                            if (dims[j] != gdims[j])
                                gdims[j] = IrregularDim;
                        }
                    }
                }
                DefineADIOS2Variable(io, m_fh->var_namelist[vi->varid], vi,
                                     gdims, joinedread, (vi->global > 0));
            }
            else /* Scalars */
            {
                /* scalar variable but global value or local value*/
                bool isChangingDim = false;
                bool isLocalValue = IsVarLocalValue(vi);
                if (isLocalValue)
                {
                    /* Local Value */
                    for (int step = 1; step < vi->nsteps; ++step)
                    {
                        if (vi->nblocks[step] != vi->nblocks[0])
                            isChangingDim = true;
                    }
                    if (isChangingDim)
                    {
                        DefineADIOS2Variable(io, m_fh->var_namelist[vi->varid],
                                             vi, {IrregularDim}, false, false);
                    }
                    else
                    {
                        DefineADIOS2Variable(io, m_fh->var_namelist[vi->varid],
                                             vi, {(unsigned int)vi->nblocks[0]},
                                             false, false);
                    }
                }
                else
                {
                    DefineADIOS2Variable(io, m_fh->var_namelist[vi->varid], vi,
                                         {}, false, true);
                }
            }
            adios_free_varinfo(vi);
        }
    }
}

void ADIOS1CommonRead::DefineADIOS2Attribute(IO &io, const char *name,
                                             enum ADIOS_DATATYPES type,
                                             void *value)
{
    switch (type)
    {
    case adios_unsigned_byte:
        DefineADIOS2Attribute<unsigned char>(io, name, value);
        break;
    case adios_unsigned_short:
        DefineADIOS2Attribute<unsigned short>(io, name, value);
        break;
    case adios_unsigned_integer:
        DefineADIOS2Attribute<unsigned int>(io, name, value);
        break;
    case adios_unsigned_long:
        DefineADIOS2Attribute<uint64_t>(io, name, value);
        break;

    case adios_byte:
        DefineADIOS2Attribute<signed char>(io, name, value);
        break;
    case adios_short:
        DefineADIOS2Attribute<short>(io, name, value);
        break;
    case adios_integer:
        DefineADIOS2Attribute<int>(io, name, value);
        break;
    case adios_long:
        DefineADIOS2Attribute<int64_t>(io, name, value);
        break;

    case adios_real:
        DefineADIOS2Attribute<float>(io, name, value);
        break;
    case adios_double:
        DefineADIOS2Attribute<double>(io, name, value);
        break;
    case adios_long_double:
        DefineADIOS2Attribute<long double>(io, name, value);
        break;

    case adios_string:
    {
        std::string str(reinterpret_cast<char *>(value));
        io.DefineAttribute<std::string>(name, str);
        break;
    }
    case adios_complex:
        /*FIXME: DefineADIOS2Attribute<std::complex<float>>(io, name, value);*/
        break;
    case adios_double_complex:
        /*FIXME: DefineADIOS2Attribute<std::complex<double>>(io, name, value);*/
        break;
    default:
        break;
    }
}

void ADIOS1CommonRead::GenerateAttributes(IO &io)
{
    if (!m_fh)
        return;
    /* Create a Variable for each variable in the file */
    for (int attid = 0; attid < m_fh->nattrs; attid++)
    {
        // here read attribute metadata (dimensions, type, etc.)...then create a
        // Variable like below:
        // Variable<T>& variable = io.DefineVariable<T>( m_Name + "/" +
        // name, )
        // return &variable; //return address if success
        enum ADIOS_DATATYPES atype;
        int asize;
        void *adata;
        int status = adios_get_attr_byid(m_fh, attid, &atype, &asize, &adata);
        if (status == err_no_error)
        {
            DefineADIOS2Attribute(io, m_fh->attr_namelist[attid], atype, adata);
            free(adata);
        }
    }
}

void ADIOS1CommonRead::ScheduleReadCommon(const std::string &name,
                                          const Dims &offs, const Dims &ldims,
                                          const int fromStep, const int nSteps,
                                          const bool readAsLocalValue,
                                          const bool readAsJoinedArray,
                                          void *data)
{
    const int adios1FromStep = fromStep;
    if (readAsLocalValue)
    {
        /* Get all the requested values from metadata now */
        ADIOS_VARINFO *vi = adios_inq_var(m_fh, name.c_str());
        if (vi)
        {
            adios_inq_var_stat(m_fh, vi, 0, 1);
            int elemsize = adios_type_size(vi->type, nullptr);
            long long blockidx = 0;
            for (int i = 0; i < adios1FromStep; i++)
            {
                blockidx += vi->nblocks[i];
            }
            char *dest = (char *)data;
            for (int i = adios1FromStep; i < adios1FromStep + nSteps; i++)
            {
                for (int j = 0; j < vi->nblocks[i]; j++)
                {
                    memcpy(dest, vi->statistics->blocks->mins[blockidx],
                           elemsize);
                    ++blockidx;
                    dest += elemsize;
                }
            }
            adios_free_varinfo(vi);
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
        adios_schedule_read(m_fh, sel, name.c_str(), (int)adios1FromStep,
                            (int)nSteps, data);
        adios_selection_delete(sel);
    }
}

void ADIOS1CommonRead::PerformReads()
{
    adios_perform_reads(m_fh, static_cast<int>(ReadMode::Blocking));
}

StepStatus ADIOS1CommonRead::AdvanceStep(IO &io, const StepMode mode,
                                         const float timeout_sec)
{
    if (m_OpenAsFile)
    {
        throw std::invalid_argument("ERROR: ADIOS1Reader does not allow "
                                    "Advance() on a file which was opened for "
                                    "read as File\n");
    }

    if (mode != StepMode::NextAvailable && mode != StepMode::LatestAvailable)
    {
        throw std::invalid_argument(
            "ERROR: ADIOS1Reader.Advance() only allows "
            "for NextAvailable or LatestAvailable modes.\n");
    }

    if (m_IsBeforeFirstStep)
    {
        /* ADIOS1 already has the first step open after Open(), in ADIOS2 we
         * wait for
         * the first call to BeginStep(), which calls this function.
         */

        m_IsBeforeFirstStep = false;
        GenerateAttributes(io);
        adios_errno = err_no_error;
    }
    else
    {
        int last = (mode == StepMode::NextAvailable ? 0 : 1);
        float *timeout = const_cast<float *>(&timeout_sec);
        adios_advance_step(m_fh, last, *timeout);
        if (adios_errno != err_step_notready)
        {
            io.RemoveAllVariables();
        }
    }

    StepStatus status;
    switch (adios_errno)
    {
    case err_no_error:
        status = StepStatus::OK;
        GenerateVariables(io);
        break;
    case err_end_of_stream:
        status = StepStatus::EndOfStream;
        break;
    case err_step_notready:
        status = StepStatus::NotReady;
        break;
    default:
        status = StepStatus::OtherError;
        break;
    }
    return status;
}

size_t ADIOS1CommonRead::CurrentStep() const
{
    return static_cast<size_t>(m_fh->current_step);
}

void ADIOS1CommonRead::ReleaseStep() { adios_release_step(m_fh); }

ADIOS_VARINFO *ADIOS1CommonRead::InqVar(const std::string &varName)
{
    return adios_inq_var(m_fh, varName.c_str());
}

void ADIOS1CommonRead::InqVarBlockInfo(ADIOS_VARINFO *vi)
{
    adios_inq_var_blockinfo(m_fh, vi);
}

void ADIOS1CommonRead::FreeVarInfo(ADIOS_VARINFO *vi) {}

bool ADIOS1CommonRead::IsVarLocalValue(ADIOS_VARINFO *vi)
{
    bool isLocalValue = false;
    std::string aname(m_fh->var_namelist[vi->varid]);
    aname = aname + "/ReadAsArray";
    for (int i = 0; i < vi->nattrs; ++i)
    {
        if (!strcmp(m_fh->attr_namelist[vi->attr_ids[i]], aname.c_str()))
        {
            isLocalValue = true;
            break;
        }
    }
    return isLocalValue;
}

void ADIOS1CommonRead::Close()
{
    if (m_IsFileOpen)
    {
        adios_read_close(m_fh);
        m_IsFileOpen = false;
    }
}

// PRIVATE
void ADIOS1CommonRead::Init()
{
    if (!m_IsInitialized)
    {
        m_IsInitialized = true;
    }
}

void ADIOS1CommonRead::InitParameters(const Params &parameters)
{
    auto itOpenAsFile = parameters.find("OpenAsFile");
    if (itOpenAsFile == parameters.end())
    {
        m_OpenAsFile = false;
    }
    else if (itOpenAsFile->second == "true")
    {
        m_OpenAsFile = true;
    }

    auto itVerbosity = parameters.find("verbose");
    if (itVerbosity != parameters.end())
    {
        int verbosity = std::stoi(itVerbosity->second);
        if (m_DebugMode)
        {
            if (verbosity < 0 || verbosity > 5)
                throw std::invalid_argument(
                    "ERROR: Method verbose argument must be an "
                    "integer in the range [0,5], in call to "
                    "Open or Engine constructor\n");
        }
        adios_verbose_level = verbosity;
    }
}

void ADIOS1CommonRead::InitTransports(
    const std::vector<Params> &transportsParameters)
{
    for (const auto &parameters : transportsParameters)
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
                    " (you mean File?) not supported, in " + m_FileName + "\n");
            }
        }
    }
    adios_read_init_method(m_ReadMethod, m_MPIComm, "");
}

/*
void ADIOS1CommonRead::DefineVariable(const std::string &name,
                                      const ShapeID shapeID,
                                      enum ADIOS_DATATYPES vartype,
                                      const std::string ldims,
                                      const std::string gdims,
                                      const std::string offsets)
{
    switch (shapeID)
    {
    case ShapeID::GlobalValue:
        adios_define_var(m_ADIOSGroup, name.c_str(), "", vartype, "", "",
"");
        break;
    case ShapeID::LocalValue:
        adios_define_var(m_ADIOSGroup, name.c_str(), "", vartype, "", "",
"");
        adios_define_attribute(m_ADIOSGroup, "ReadAsArray", name.c_str(),
                               adios_byte, "1", nullptr);
        break;
    case ShapeID::GlobalArray:
    case ShapeID::LocalArray:
    case ShapeID::JoinedArray:
        adios_define_var(m_ADIOSGroup, name.c_str(), "", vartype,
ldims.c_str(),
                         gdims.c_str(), offsets.c_str());
        break;
    }
}*/

/*
// Explicit declaration of the public template methods
#define declare_template_instantiation(T) \
    template void ADIOS1CommonRead::ReadVariable<T>( \
        const std::string &name, const ShapeID shapeID, const Dims ldims, \
        const Dims gdims, const Dims offsets, const T *values);
ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
*/

} // end namespace interop
} // end namespace adios
