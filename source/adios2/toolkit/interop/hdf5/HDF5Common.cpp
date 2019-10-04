/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5Common.cpp
 *
 *  Created on: April 20, 2017
 *      Author: Junmin
 */

#include "HDF5Common.h"
#include "HDF5Common.tcc"

#include <complex>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "adios2/helper/adiosFunctions.h" // IsRowMajor
#include <cstring>                        // strlen

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#endif

namespace adios2
{
namespace interop
{

const std::string HDF5Common::ATTRNAME_NUM_STEPS = "NumSteps";
const std::string HDF5Common::ATTRNAME_GIVEN_ADIOSNAME = "ADIOSName";
const std::string HDF5Common::PREFIX_BLOCKINFO = "ADIOS_BLOCKINFO_";
const std::string HDF5Common::PREFIX_STAT = "ADIOS_STAT_";
const std::string HDF5Common::PARAMETER_COLLECTIVE = "H5CollectiveMPIO";
const std::string HDF5Common::PARAMETER_CHUNK_FLAG = "H5ChunkDim";
const std::string HDF5Common::PARAMETER_CHUNK_VARS = "H5ChunkVars";

/*
   //need to know ndim before defining this.
   //inconvenient
class HDF5BlockStat
{
public:
  size_t m_offset[ndim];
  size_t m_Count[ndim];
  double m_Min;
  double m_Max;
};
*/

HDF5Common::HDF5Common(const bool debugMode) : m_DebugMode(debugMode)
{
    m_DefH5TypeComplexFloat =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<float>));
    H5Tinsert(m_DefH5TypeComplexFloat, "freal", 0, H5T_NATIVE_FLOAT);
    H5Tinsert(m_DefH5TypeComplexFloat, "fimg", H5Tget_size(H5T_NATIVE_FLOAT),
              H5T_NATIVE_FLOAT);

    m_DefH5TypeComplexDouble =
        H5Tcreate(H5T_COMPOUND, sizeof(std::complex<double>));
    H5Tinsert(m_DefH5TypeComplexDouble, "dreal", 0, H5T_NATIVE_DOUBLE);
    H5Tinsert(m_DefH5TypeComplexDouble, "dimg", H5Tget_size(H5T_NATIVE_DOUBLE),
              H5T_NATIVE_DOUBLE);

    m_PropertyTxfID = H5Pcreate(H5P_DATASET_XFER);
}

void HDF5Common::ParseParameters(core::IO &io)
{
#ifdef ADIOS2_HAVE_MPI
    auto itKey = io.m_Parameters.find(PARAMETER_COLLECTIVE);
    if (itKey != io.m_Parameters.end())
    {
        if (itKey->second == "yes" || itKey->second == "true")
            H5Pset_dxpl_mpio(m_PropertyTxfID, H5FD_MPIO_COLLECTIVE);
    }
#endif

    m_ChunkVarNames.clear();
    m_ChunkPID = -1;
    m_ChunkDim = 0;

    {
        std::vector<hsize_t> chunkDim;
        auto chunkFlagKey = io.m_Parameters.find(PARAMETER_CHUNK_FLAG);
        if (chunkFlagKey != io.m_Parameters.end())
        { // note space is the delimiter
            std::stringstream ss(chunkFlagKey->second);
            int i;
            while (ss >> i)
                chunkDim.push_back(i);

            m_ChunkPID = H5Pcreate(H5P_DATASET_CREATE);
            m_ChunkDim = chunkDim.size();
            if (m_ChunkDim > 0)
                H5Pset_chunk(m_ChunkPID, chunkDim.size(), chunkDim.data());
        }
    }

    //
    // if no chunk dim specified, then ignore this parameter
    //
    if (-1 != m_ChunkPID)
    {
        auto chunkVarKey = io.m_Parameters.find(PARAMETER_CHUNK_VARS);
        if (chunkVarKey != io.m_Parameters.end())
        {
            std::stringstream ss(chunkVarKey->second);
            std::string token;
            while (ss >> token)
                m_ChunkVarNames.insert(token);
        }
    }
}

void HDF5Common::Init(const std::string &name, helper::Comm const &comm,
                      bool toWrite)
{
    m_WriteMode = toWrite;
    m_PropertyListId = H5Pcreate(H5P_FILE_ACCESS);

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm mpiComm = helper::CommAsMPI(comm);
    if (mpiComm != MPI_COMM_NULL)
    {
        MPI_Comm_rank(mpiComm, &m_CommRank);
        MPI_Comm_size(mpiComm, &m_CommSize);
        if (m_CommSize != 1)
        {
            H5Pset_fapl_mpio(m_PropertyListId, mpiComm, MPI_INFO_NULL);
        }
    }
    else
    {
        m_CommRank = 0;
        m_CommSize = 1;
    }
#endif

    // std::string ts0 = "/AdiosStep0";
    std::string ts0;
    StaticGetAdiosStepString(ts0, 0);

    if (toWrite)
    {
        /*
         * Create a new file collectively and release property list identifier.
         */
        m_FileId = H5Fcreate(name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
                             m_PropertyListId);
        if (m_FileId >= 0)
        {
            m_GroupId = H5Gcreate2(m_FileId, ts0.c_str(), H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT);

            if (m_DebugMode)
            {
                if (m_GroupId < 0)
                {
                    throw std::ios_base::failure(
                        "ERROR: Unable to create HDF5 group " + ts0 +
                        " in call to Open\n");
                }
            }
        }
    }
    else
    {
        // read a file collectively
        m_FileId = H5Fopen(name.c_str(), H5F_ACC_RDONLY, m_PropertyListId);
        if (m_FileId >= 0)
        {
            if (H5Lexists(m_FileId, ts0.c_str(), H5P_DEFAULT) != 0)
            {
                m_GroupId = H5Gopen(m_FileId, ts0.c_str(), H5P_DEFAULT);
                m_IsGeneratedByAdios = true;
            }
        }
    }

    H5Pclose(m_PropertyListId);
}

void HDF5Common::WriteAdiosSteps()
{
    if (m_FileId < 0)
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: invalid HDF5 file to record "
                                        "steps, in call to Write\n");
        }
    }

    if (!m_WriteMode)
    {
        return;
    }

    hid_t s = H5Screate(H5S_SCALAR);
    hid_t attr =
        H5Acreate(m_FileId, ATTRNAME_NUM_STEPS.c_str(),
                  /*"NumSteps",*/ H5T_NATIVE_UINT, s, H5P_DEFAULT, H5P_DEFAULT);
    unsigned int totalAdiosSteps = m_CurrentAdiosStep + 1;

    if (m_GroupId < 0)
    {
        totalAdiosSteps = m_CurrentAdiosStep;
    }

    H5Awrite(attr, H5T_NATIVE_UINT, &totalAdiosSteps);

    H5Sclose(s);
    H5Aclose(attr);
}

unsigned int HDF5Common::GetNumAdiosSteps()
{
    if (m_WriteMode)
    {
        return static_cast<unsigned int>(-1);
    }

    if (m_FileId < 0)
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: invalid HDF5 file to read step attribute.\n");
        }
    }

    if (!m_IsGeneratedByAdios)
    {
        return 1;
    }

    if (m_NumAdiosSteps <= 0)
    {
        hsize_t numobj;
        H5Gget_num_objs(m_FileId, &numobj);
        m_NumAdiosSteps = numobj;

        if (H5Aexists(m_FileId, ATTRNAME_NUM_STEPS.c_str()))
        {
            hid_t attr =
                H5Aopen(m_FileId, ATTRNAME_NUM_STEPS.c_str(), H5P_DEFAULT);

            H5Aread(attr, H5T_NATIVE_UINT, &m_NumAdiosSteps);
            H5Aclose(attr);
        }
    }

    return m_NumAdiosSteps;
}

// read from all time steps
void HDF5Common::ReadAllVariables(core::IO &io)
{
    if (!m_IsGeneratedByAdios)
    {
        FindVarsFromH5(io, m_FileId, "/", "", 0);
        return;
    }

    GetNumAdiosSteps();
    int i = 0;

    for (i = 0; i < m_NumAdiosSteps; i++)
    {
        ReadVariables(i, io);
    }
}

void HDF5Common::FindVarsFromH5(core::IO &io, hid_t top_id, const char *gname,
                                const char *heritage, unsigned int ts)
{
    // int i = 0;
    // std::string stepStr;
    hsize_t numObj;

    // StaticGetAdiosStepString(stepStr, ts);
    hid_t gid = H5Gopen2(top_id, gname, H5P_DEFAULT);
    HDF5TypeGuard g(gid, E_H5_GROUP);
    ///    if (gid > 0) {
    herr_t ret = H5Gget_num_objs(gid, &numObj);
    if (ret >= 0)
    {
        int k = 0;
        char name[100];
        for (k = 0; k < numObj; k++)
        {
            ret = H5Gget_objname_by_idx(gid, (hsize_t)k, name, sizeof(name));
            if (ret >= 0)
            {
                int currType = H5Gget_objtype_by_idx(gid, k);
                if ((currType == H5G_DATASET) || (currType == H5G_TYPE))
                {
                    std::string nameStr = name;
                    // if (!(0 == nameStr.find(PREFIX_BLOCKINFO)) &&
                    //  !(0 == nameStr.find(PREFIX_STAT)))
                    // cave in to cadacity requirement to pass pull request.
                    // This is odd
                    if (std::string::npos == nameStr.find(PREFIX_BLOCKINFO) &&
                        std::string::npos == nameStr.find(PREFIX_STAT))
                    {
                        hid_t datasetId = H5Dopen(gid, name, H5P_DEFAULT);
                        HDF5TypeGuard d(datasetId, E_H5_DATASET);

                        std::string longName;

                        if (strcmp(gname, "/") == 0)
                        {
                            longName = std::string("/") + name;
                        }
                        else
                        {
                            longName = std::string(heritage) + "/" + gname +
                                       "/" + name;
                        }
                        // CreateVar(io, datasetId, name);
                        ReadNativeAttrToIO(io, datasetId, longName);
                        CreateVar(io, datasetId, longName, ts);
                    }
                }
                else if (currType == H5G_GROUP)
                {
                    std::string heritageNext = heritage;
                    if (top_id != m_FileId)
                    {
                        heritageNext += "/";
                        heritageNext += gname;
                    }
                    FindVarsFromH5(io, gid, name, heritageNext.c_str(), ts);
                }
            }
        }
    }
}
/*
void HDF5Common::ReadNativeGroup(hid_t hid, IO& io)
{
H5G_info_t group_info;
herr_t result = H5Gget_info(hid, &group_info);

if (result < 0) {
  // error
  throw std::ios_base::failure("Unable to get group info.");
}

if (group_info.nlinks == 0) {
  return;
}

char tmpstr[1024];

hsize_t idx;
for (idx=0; idx<group_info.nlinks; idx++) {
  int currType = H5Gget_objtype_by_idx(hid, idx);
  if (currType < 0) {
    throw std::ios_base::failure("unable to get type info of idx"+idx);
  }


  ssize_t curr= H5Gget_objname_by_idx(hid, idx, tmpstr, sizeof(tmpstr));
  if (curr > 0) { // got a name
    std::cout<<" ... printing a name: "<<tmpstr<<",
type:[0=G/1=D/2=T/3=L/4=UDL]"<<currType<<std::endl;
  }
}
}
*/

// read variables from the input timestep
void HDF5Common::ReadVariables(unsigned int ts, core::IO &io)
{
    std::string stepStr;
    hsize_t numObj;

    StaticGetAdiosStepString(stepStr, ts);
    hid_t gid = H5Gopen2(m_FileId, stepStr.c_str(), H5P_DEFAULT);
    HDF5TypeGuard g(gid, E_H5_GROUP);
    ///    if (gid > 0) {
    herr_t ret = H5Gget_num_objs(gid, &numObj);
    if (ret >= 0)
    {
        int k = 0;
        char name[50];
        for (k = 0; k < numObj; k++)
        {
            ret = H5Gget_objname_by_idx(gid, (hsize_t)k, name, sizeof(name));
            if (ret >= 0)
            {
                int currType = H5Gget_objtype_by_idx(gid, k);
                if (currType == H5G_GROUP)
                {
                    FindVarsFromH5(io, gid, name, "", ts);
                }
                else if ((currType == H5G_DATASET) || (currType == H5G_TYPE))
                {
                    std::string nameStr = name;
                    // if (!(0 == nameStr.find(PREFIX_BLOCKINFO)) &&
                    //  !(0 == nameStr.find(PREFIX_STAT)))
                    // cave in to cadacity requirement to pass pull request.
                    // This is odd
                    if (std::string::npos == nameStr.find(PREFIX_BLOCKINFO) &&
                        std::string::npos == nameStr.find(PREFIX_STAT))

                    {
                        hid_t datasetId = H5Dopen(gid, name, H5P_DEFAULT);
                        HDF5TypeGuard d(datasetId, E_H5_DATASET);
                        ReadNativeAttrToIO(io, datasetId, name);
                        CreateVar(io, datasetId, name, ts);
                    }
                }
            }
        }
    }
    /// H5Gclose(gid);
    ///}
}

template <class T>
void HDF5Common::AddVar(core::IO &io, std::string const &name, hid_t datasetId,
                        unsigned int ts)
{
    core::Variable<T> *v = io.InquireVariable<T>(name);
    if (NULL == v)
    {
        hid_t dspace = H5Dget_space(datasetId);
        const int ndims = H5Sget_simple_extent_ndims(dspace);
        std::vector<hsize_t> dims(ndims);
        H5Sget_simple_extent_dims(dspace, dims.data(), NULL);
        H5Sclose(dspace);

        Dims shape;
        shape.resize(ndims);
        if (ndims > 0)
        {
            bool isOrderC = helper::IsRowMajor(io.m_HostLanguage);
            for (int i = 0; i < ndims; i++)
            {
                if (isOrderC)
                {
                    shape[i] = dims[i];
                }
                else
                {
                    shape[i] = dims[ndims - 1 - i];
                }
            }
        }

        Dims zeros(shape.size(), 0);

        try
        {
            auto &foo = io.DefineVariable<T>(name, shape, zeros, shape);
            // 0 is a dummy holder. Just to make sure the ts entry is in there
            foo.m_AvailableStepBlockIndexOffsets[ts + 1] =
                std::vector<size_t>({0});
            foo.m_AvailableStepsStart = ts;
            // default was set to 0 while m_AvailabelStepsStart is 1.
            // correcting

            if (0 == foo.m_AvailableStepsCount)
            {
                foo.m_AvailableStepsCount++;
            }
        }
        catch (std::exception &e)
        {
            // invalid variable, do not define
            printf("WARNING: IO is not accepting definition of variable: %s. "
                   "Skipping. \n",
                   name.c_str());
        }
    }
    else
    {
        /*    if (0 == v->m_AvailableStepsCount) { // default was set to 0 while
        m_AvailabelStepsStart is 1. v->m_AvailableStepsCount ++;
        }
        */
        v->m_AvailableStepsCount++;
        v->m_AvailableStepBlockIndexOffsets[ts + 1] = std::vector<size_t>({0});
    }
}

void HDF5Common::CreateVar(core::IO &io, hid_t datasetId,
                           std::string const &nameSuggested, unsigned int ts)
{
    std::string name;
    ReadADIOSName(datasetId, name);
    if (name.size() == 0)
    {
        name = nameSuggested;
    }

    hid_t h5Type = H5Dget_type(datasetId);
    HDF5TypeGuard t(h5Type, E_H5_DATATYPE);

    // int8 is mapped to "signed char" by IO.
    // so signed needs to be checked before char, otherwise type is char instead
    // of "signed char". Inqvar considers "signed char" and "char" different
    // types and returns error

    if (H5Tget_class(h5Type) == H5T_STRING)
    // if (H5Tequal(H5T_STRING, h5Type))
    {
        AddVar<std::string>(io, name, datasetId, ts);
        return;
    }

    if (H5Tequal(H5T_NATIVE_INT8, h5Type))
    {
        AddVar<int8_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_UINT8, h5Type))
    {
        AddVar<uint8_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_INT16, h5Type))
    {
        AddVar<int16_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_UINT16, h5Type))
    {
        AddVar<uint16_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_INT32, h5Type))
    {
        AddVar<int32_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_UINT32, h5Type))
    {
        AddVar<uint32_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_INT64, h5Type))
    {
        AddVar<int64_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_UINT64, h5Type))
    {
        AddVar<uint64_t>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_FLOAT, h5Type))
    {
        AddVar<float>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_DOUBLE, h5Type))
    {
        AddVar<double>(io, name, datasetId, ts);
    }
    else if (H5Tequal(H5T_NATIVE_LDOUBLE, h5Type))
    {
        AddVar<long double>(io, name, datasetId, ts);
    }
    else if (H5Tequal(m_DefH5TypeComplexFloat, h5Type))
    {
        AddVar<std::complex<float>>(io, name, datasetId, ts);
    }
    else if (H5Tequal(m_DefH5TypeComplexDouble, h5Type))
    {
        AddVar<std::complex<double>>(io, name, datasetId, ts);
    }

    // H5Tclose(h5Type);
}

void HDF5Common::Close()
{
    if (m_FileId < 0)
    {
        return;
    }

    WriteAdiosSteps();

    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
    }

    H5Pclose(m_PropertyTxfID);
    H5Fclose(m_FileId);
    if (-1 != m_ChunkPID)
        H5Pclose(m_ChunkPID);

    m_FileId = -1;
    m_GroupId = -1;
}

void HDF5Common::SetAdiosStep(int step)
{
    if (m_WriteMode)
        throw std::ios_base::failure(
            "ERROR: unable to change step at Write MODE.");

    if (step < 0)
        throw std::ios_base::failure(
            "ERROR: unable to change to negative step.");

    GetNumAdiosSteps();

    if (step >= m_NumAdiosSteps)
        throw std::ios_base::failure(
            "ERROR: given time step is more than actual known steps.");

    if (m_CurrentAdiosStep == step)
    {
        return;
    }

    std::string stepName;
    StaticGetAdiosStepString(stepName, step);
    m_GroupId = H5Gopen(m_FileId, stepName.c_str(), H5P_DEFAULT);
    if (m_GroupId < 0)
    {
        throw std::ios_base::failure("ERROR: unable to open HDF5 group " +
                                     stepName + ", in call to Open\n");
    }

    m_CurrentAdiosStep = step;
}

void HDF5Common::Advance()
{
    if (m_GroupId >= 0)
    {
        H5Gclose(m_GroupId);
        m_GroupId = -1;
    }

    if (m_WriteMode)
    {
        // m_GroupId = H5Gcreate2(m_FileId, tsname.c_str(), H5P_DEFAULT,
        //                       H5P_DEFAULT, H5P_DEFAULT);
    }
    else
    {
        if (m_NumAdiosSteps == 0)
        {
            GetNumAdiosSteps();
        }
        if (m_CurrentAdiosStep + 1 >= m_NumAdiosSteps)
        {
            return;
        }

        // std::string stepName =
        //    "/AdiosStep" + std::to_string(m_CurrentAdiosStep + 1);
        std::string stepName;
        StaticGetAdiosStepString(stepName, m_CurrentAdiosStep + 1);
        m_GroupId = H5Gopen(m_FileId, stepName.c_str(), H5P_DEFAULT);
        if (m_GroupId < 0)
        {
            throw std::ios_base::failure("ERROR: unable to open HDF5 group " +
                                         stepName + ", in call to Open\n");
        }
    }
    ++m_CurrentAdiosStep;
}

void HDF5Common::CheckWriteGroup()
{
    if (!m_WriteMode)
    {
        return;
    }
    if (m_GroupId >= 0)
    {
        return;
    }

    // std::string stepName = "/AdiosStep" +
    // std::to_string(m_CurrentAdiosStep);
    std::string stepName;
    StaticGetAdiosStepString(stepName, m_CurrentAdiosStep);
    m_GroupId = H5Gcreate2(m_FileId, stepName.c_str(), H5P_DEFAULT, H5P_DEFAULT,
                           H5P_DEFAULT);

    if (m_DebugMode)
    {
        if (m_GroupId < 0)
        {
            throw std::ios_base::failure(
                "ERROR: HDF5: Unable to create group " + stepName);
        }
    }
}

void HDF5Common::ReadStringScalarDataset(hid_t dataSetId, std::string &result)
{
    hid_t h5Type = H5Dget_type(dataSetId); // get actual type;
    size_t typesize = H5Tget_size(h5Type);

    char *val = (char *)(calloc(typesize, sizeof(char)));
    hid_t ret2 = H5Dread(dataSetId, h5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT, val);

    result.assign(val, typesize);
    free(val);

    H5Tclose(h5Type);
}

hid_t HDF5Common::GetTypeStringScalar(const std::string &input)
{
    /* Create a datatype to refer to. */
    hid_t type = H5Tcopy(H5T_C_S1);
    hid_t ret = H5Tset_size(type, input.size());

    ret = H5Tset_strpad(type, H5T_STR_NULLTERM);

    return type;
}

void HDF5Common::CreateDataset(const std::string &varName, hid_t h5Type,
                               hid_t filespaceID,
                               std::vector<hid_t> &datasetChain)
{
    std::vector<std::string> list;
    char delimiter = '/';
    int delimiterLength = 1;
    std::string s = std::string(varName);
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        if (pos > 0)
        { // "///a/b/c" == "a/b/c"
            token = s.substr(0, pos);
            list.push_back(token);
        }
        s.erase(0, pos + delimiterLength);
    }
    list.push_back(s);

    hid_t topId = m_GroupId;
    if (list.size() > 1)
    {
        for (int i = 0; i < list.size() - 1; i++)
        {
            if (H5Lexists(topId, list[i].c_str(), H5P_DEFAULT) == 0)
            { // does not exist, so create
                topId = H5Gcreate2(topId, list[i].c_str(), H5P_DEFAULT,
                                   H5P_DEFAULT, H5P_DEFAULT);
            }
            else
            {
                topId = H5Gopen(topId, list[i].c_str(), H5P_DEFAULT);
            }
            datasetChain.push_back(topId);
        }
    }

    hid_t varCreateProperty = H5P_DEFAULT;
    if (-1 != m_ChunkPID)
    {
        if (m_ChunkVarNames.size() == 0) // applies to all var
            varCreateProperty = m_ChunkPID;
        else if (m_ChunkVarNames.find(varName) != m_ChunkVarNames.end())
            varCreateProperty = m_ChunkPID;
    }

    hid_t dsetID = H5Dcreate(topId, list.back().c_str(), h5Type, filespaceID,
                             H5P_DEFAULT, varCreateProperty, H5P_DEFAULT);

    if (list.back().compare(varName) != 0)
    {
        StoreADIOSName(varName, dsetID); // only stores when not the same
    }
    datasetChain.push_back(dsetID);
    // return dsetID;
}

void HDF5Common::StoreADIOSName(const std::string adiosName, hid_t dsetID)
{
    hid_t attrSpace = H5Screate(H5S_SCALAR);
    hid_t atype = H5Tcopy(H5T_C_S1);
    H5Tset_size(atype, adiosName.size());
    H5Tset_strpad(atype, H5T_STR_NULLTERM);
    hid_t attr = H5Acreate2(dsetID, ATTRNAME_GIVEN_ADIOSNAME.c_str(), atype,
                            attrSpace, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attr, atype, adiosName.c_str());

    H5Sclose(attrSpace);
    H5Tclose(atype);
    H5Aclose(attr);
}

void HDF5Common::ReadADIOSName(hid_t dsetID, std::string &adiosName)
{
    // htri_t H5Lexists( hid_t loc_id, const char *name, hid_t lapl_id )
    if (H5Aexists(dsetID, ATTRNAME_GIVEN_ADIOSNAME.c_str()) <= 0)
    {
        return;
    }

    hid_t attrID =
        H5Aopen(dsetID, ATTRNAME_GIVEN_ADIOSNAME.c_str(), H5P_DEFAULT);
    if (attrID < 0)
    {
        return;
    }

    hid_t attrType = H5Aget_type(attrID);
    size_t typeSize = H5Tget_size(attrType);

    char *val = (char *)(calloc(typeSize, sizeof(char)));
    hid_t ret2 = H5Aread(attrID, attrType, val);

    H5Tclose(attrType);
    H5Aclose(attrID);

    adiosName.assign(val, typeSize);
    free(val);
}

bool HDF5Common::OpenDataset(const std::string &varName,
                             std::vector<hid_t> &datasetChain)
{
    std::vector<std::string> list;
    char delimiter = '/';
    int delimiterLength = 1;
    std::string s = std::string(varName);
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        if (pos > 0)
        { // "///a/b/c" == "a/b/c"
            token = s.substr(0, pos);
            list.push_back(token);
        }
        s.erase(0, pos + delimiterLength);
    }
    list.push_back(s);

    if (list.size() == 1)
    {
        hid_t dsetID = H5Dopen(m_GroupId, list[0].c_str(), H5P_DEFAULT);
        datasetChain.push_back(dsetID);
        return true;
    }

    hid_t topId = m_GroupId;

    for (int i = 0; i < list.size() - 1; i++)
    {
        if (H5Lexists(topId, list[i].c_str(), H5P_DEFAULT) == 0)
        { // does not exist, err
            // topId = H5Gcreate2(topId, list[i].c_str(), H5P_DEFAULT,
            // H5P_DEFAULT,H5P_DEFAULT);
            printf("Unable to open HDF5 group: %s for %s. Quit. \n",
                   list[i].c_str(), varName.c_str());
            return false;
        }
        else
        {
            topId = H5Gopen(topId, list[i].c_str(), H5P_DEFAULT);
        }
        datasetChain.push_back(topId);
    }

    hid_t dsetID = H5Dopen(topId, list.back().c_str(), H5P_DEFAULT);

    datasetChain.push_back(dsetID);
    return true;
}

// trim from right
inline std::string &rtrim(std::string &s, const char *t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left
inline std::string &ltrim(std::string &s, const char *t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from left & right
inline std::string &trim(std::string &s, const char *t = " \t\n\r\f\v")
{
    return ltrim(rtrim(s, t), t);
}

void HDF5Common::ReadInStringAttr(core::IO &io, const std::string &attrName,
                                  hid_t attrId, hid_t h5Type, hid_t sid)
{
    hsize_t typeSize = H5Tget_size(h5Type);
    H5S_class_t stype = H5Sget_simple_extent_type(sid);

    if (H5S_SCALAR == stype)
    {
        auto val = std::unique_ptr<char[]>(new char[typeSize]);
        H5Aread(attrId, h5Type, &val[0]);

        auto strValue = std::string(&val[0], typeSize);
        io.DefineAttribute<std::string>(attrName, strValue);
    }
    else
    { // array
        hsize_t ndims = H5Sget_simple_extent_ndims(sid);
        if (ndims != 1)
        {
            return; // so far io can only handle 1-D array
        }
        // ndims must be 1
        hsize_t dims[1];
        hid_t ret = H5Sget_simple_extent_dims(sid, dims, NULL);

        auto val = std::unique_ptr<char[]>(new char[typeSize * dims[0]]);
        H5Aread(attrId, h5Type, val.get());

        std::vector<std::string> stringArray;
        for (int i = 0; i < dims[0]; i++)
        {
            auto input = std::string(&val[i * typeSize], typeSize);
            // remove the padded empty space;
            rtrim(input);
            stringArray.push_back(input);
        }

        io.DefineAttribute<std::string>(attrName, stringArray.data(), dims[0]);
    }
}

template <class T>
void HDF5Common::AddNonStringAttribute(core::IO &io,
                                       std::string const &attrName,
                                       hid_t attrId, hid_t h5Type,
                                       hsize_t arraySize)
{
    if (arraySize == 0)
    { // SCALAR
        T val;
        H5Aread(attrId, h5Type, &val);
        io.DefineAttribute(attrName, val);
    }
    else
    {
        std::vector<T> val(arraySize);
        H5Aread(attrId, h5Type, val.data());
        io.DefineAttribute(attrName, val.data(), arraySize);
    }
}

void HDF5Common::ReadInNonStringAttr(core::IO &io, const std::string &attrName,
                                     hid_t attrId, hid_t h5Type, hid_t sid)
{
    H5S_class_t stype = H5Sget_simple_extent_type(sid);

    hsize_t ndims = H5Sget_simple_extent_ndims(sid);
    size_t typesize = H5Tget_size(h5Type);

    if (ndims > 1)
    {
        return; // so far adios2 io can only handle 1-D array
    }

    hsize_t dims[1];
    dims[0] = 0;
    if (ndims == 1)
        hid_t ret = H5Sget_simple_extent_dims(sid, dims, NULL);

    if (H5Tequal(H5T_NATIVE_INT8, h5Type))
    {
        AddNonStringAttribute<int8_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_UINT8, h5Type))
    {
        AddNonStringAttribute<uint8_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_INT16, h5Type))
    {
        AddNonStringAttribute<int16_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_UINT16, h5Type))
    {
        AddNonStringAttribute<uint16_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_INT32, h5Type))
    {
        AddNonStringAttribute<int32_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_UINT32, h5Type))
    {
        AddNonStringAttribute<uint32_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_INT64, h5Type))
    {
        AddNonStringAttribute<int64_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_UINT64, h5Type))
    {
        AddNonStringAttribute<uint64_t>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_FLOAT, h5Type))
    {
        AddNonStringAttribute<float>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_DOUBLE, h5Type))
    {
        AddNonStringAttribute<double>(io, attrName, attrId, h5Type, dims[0]);
    }
    else if (H5Tequal(H5T_NATIVE_LDOUBLE, h5Type))
    {
        AddNonStringAttribute<long double>(io, attrName, attrId, h5Type,
                                           dims[0]);
    }
}

void HDF5Common::WriteStringAttr(core::IO &io,
                                 core::Attribute<std::string> *adiosAttr,
                                 const std::string &attrName, hid_t parentID)
{
    // core::Attribute<std::string> *adiosAttr =
    // io.InquireAttribute<std::string>(attrName);

    if (adiosAttr == NULL)
    {
        return;
    }

    if (adiosAttr->m_IsSingleValue)
    {
        hid_t h5Type = GetTypeStringScalar(adiosAttr->m_DataSingleValue.data());
        hid_t s = H5Screate(H5S_SCALAR);
        hid_t attr = H5Acreate2(parentID, attrName.c_str(), h5Type, s,
                                H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, h5Type, (adiosAttr->m_DataSingleValue.data()));
        H5Sclose(s);
        H5Tclose(h5Type);
        H5Aclose(attr);
    }
    else if (adiosAttr->m_Elements >= 1)
    {
        // is array
        int max = 0;
        int idxWithMax = 0;
        for (int i = 0; i < adiosAttr->m_Elements; i++)
        {
            int curr = adiosAttr->m_DataArray[i].size();
            if (max < curr)
            {
                max = curr;
                idxWithMax = i;
            }
        }

        hid_t h5Type = GetTypeStringScalar(adiosAttr->m_DataArray[idxWithMax]);
        // std::vector<char> temp;
        std::string all;
        for (int i = 0; i < adiosAttr->m_Elements; i++)
        {
            std::string curr = adiosAttr->m_DataArray[i];
            curr.resize(max, ' ');
            all.append(curr);
        }

        hsize_t onedim[1] = {adiosAttr->m_Elements};
        hid_t s = H5Screate_simple(1, onedim, NULL);
        hid_t attr = H5Acreate2(parentID, attrName.c_str(), h5Type, s,
                                H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, h5Type, all.c_str());
        H5Sclose(s);
        H5Aclose(attr);
        H5Tclose(h5Type);
    }
}

template <class T>
void HDF5Common::WriteNonStringAttr(core::IO &io, core::Attribute<T> *adiosAttr,
                                    hid_t parentID, const char *h5AttrName)
{
    if (adiosAttr == NULL)
    {
        return;
    }
    hid_t h5Type = GetHDF5Type<T>();
    if (adiosAttr->m_IsSingleValue)
    {
        hid_t s = H5Screate(H5S_SCALAR);
        // hid_t attr = H5Acreate2(parentID, adiosAttr->m_Name.c_str(), h5Type,
        // s,
        hid_t attr = H5Acreate2(parentID, h5AttrName, h5Type, s, H5P_DEFAULT,
                                H5P_DEFAULT);
        H5Awrite(attr, h5Type, &(adiosAttr->m_DataSingleValue));
        H5Sclose(s);
        H5Aclose(attr);
    }
    else if (adiosAttr->m_Elements >= 1)
    {
        hsize_t onedim[1] = {adiosAttr->m_Elements};
        hid_t s = H5Screate_simple(1, onedim, NULL);
        // hid_t attr = H5Acreate2(parentID, adiosAttr->m_Name.c_str(), h5Type,
        // s,
        hid_t attr = H5Acreate2(parentID, h5AttrName, h5Type, s, H5P_DEFAULT,
                                H5P_DEFAULT);
        H5Awrite(attr, h5Type, adiosAttr->m_DataArray.data());
        H5Sclose(s);
        H5Aclose(attr);
    }
}

void HDF5Common::LocateAttrParent(const std::string &attrName,
                                  std::vector<std::string> &list,
                                  std::vector<hid_t> &parentChain)
{
    char delimiter = '/';
    int delimiterLength = 1;
    std::string s = std::string(attrName);
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        if (pos > 0)
        { // "///a/b/c" == "a/b/c"
            token = s.substr(0, pos);
            list.push_back(token);
        }
        s.erase(0, pos + delimiterLength);
    }
    list.push_back(s);

    if (list.size() == 1)
    {
        return;
    }

    hid_t topId = m_FileId;
    if (list.size() >= 1)
    {
        std::string ts;
        for (int i = 0; i < m_CurrentAdiosStep; i++)
        {
            StaticGetAdiosStepString(ts, i);
            for (int j = 0; j < list.size() - 1; j++)
            {
                ts += delimiter;
                ts += list[j].c_str();
            }
            if (H5Lexists(m_FileId, ts.c_str(), H5P_DEFAULT) <= 0)
                continue;
            else
            {
                topId = H5Dopen(m_FileId, ts.c_str(), H5P_DEFAULT);
                break;
            }
        } // for

        if (topId != m_FileId)
            parentChain.push_back(topId);
        return;
    } // if

    // hid_t dsetID = H5Dopen(topId, list.back().c_str(), H5P_DEFAULT);

    // parentChain.push_back(dsetID);
    // return dsetID;
}

//
// write attr from io to hdf5
// right now adios only support global attr
// var does not have attr
//
void HDF5Common::WriteAttrFromIO(core::IO &io)
{
    if (m_FileId < 0)
    {
        return;
    }
    if (!m_WriteMode)
    {
        return;
    }

    const std::map<std::string, Params> &attributesInfo =
        io.GetAvailableAttributes();

    for (const auto &apair : attributesInfo)
    {
        std::string attrName = apair.first;
        Params temp = apair.second;
        std::string attrType = temp["Type"];

        hid_t parentID = m_FileId;
#ifdef NO_ATTR_VAR_ASSOC
        std::vector<hid_t> chain;
        std::vector<std::string> list;
        LocateAttrParent(attrName, list, chain);
        HDF5DatasetGuard g(chain);

        if (chain.size() > 0)
        {
            parentID = chain.back();
        }
#else
        // will list out all attr at root level
        // to make it easy to be consistant with ADIOS2 attr symantic
        std::vector<std::string> list;
        list.push_back(attrName);
#endif
        // if (H5Aexists(parentID, attrName.c_str()) > 0)
        if (H5Aexists(parentID, list.back().c_str()) > 0)
        {
            continue;
        }

        if (attrType == "compound")
        {
            // not supported
        }
        else if (attrType == helper::GetType<std::string>())
        {
            // WriteStringAttr(io, attrName, parentID);
            core::Attribute<std::string> *adiosAttr =
                io.InquireAttribute<std::string>(attrName);
            WriteStringAttr(io, adiosAttr, list.back(), parentID);
        }
//
// note no std::complext attr types
//
#define declare_template_instantiation(T)                                      \
    else if (attrType == helper::GetType<T>())                                 \
    {                                                                          \
        core::Attribute<T> *adiosAttr = io.InquireAttribute<T>(attrName);      \
        WriteNonStringAttr(io, adiosAttr, parentID, list.back().c_str());      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    // std::string attrType = attributesInfo[attrName]["Type"];
}

//
// read attr from hdf5 to IO
// only the global ones are retrieved for now,
// until adios2 starts to support var level attrs
//
void HDF5Common::ReadAttrToIO(core::IO &io)
{
    hsize_t numAttrs;
    // herr_t ret = H5Gget_num_objs(m_FileId, &numObj);
    H5O_info_t oinfo;
    herr_t ret = H5Oget_info(m_FileId, &oinfo);
    if (ret >= 0)
    {
        numAttrs = oinfo.num_attrs;
        int k = 0;
        const int MAX_ATTR_NAME_SIZE = 100;
        for (k = 0; k < numAttrs; k++)
        {
            char attrName[MAX_ATTR_NAME_SIZE];
            ret = (herr_t)H5Aget_name_by_idx(
                m_FileId, ".", H5_INDEX_CRT_ORDER, H5_ITER_DEC, (hsize_t)k,
                attrName, (size_t)MAX_ATTR_NAME_SIZE, H5P_DEFAULT);
            if (ret >= 0)
            {
                // if (strcmp(attrName, ATTRNAME_NUM_STEPS.c_str()) == 0) {
                if (ATTRNAME_NUM_STEPS.compare(attrName) == 0)
                {
                    continue;
                }

                hid_t attrId = H5Aopen(m_FileId, attrName, H5P_DEFAULT);
                if (attrId < 0)
                {
                    continue;
                }
                hid_t sid = H5Aget_space(attrId);
                H5S_class_t stype = H5Sget_simple_extent_type(sid);

                hid_t attrType = H5Aget_type(attrId);
                bool isString = (H5Tget_class(attrType) == H5T_STRING);
                if (isString)
                {
                    ReadInStringAttr(io, attrName, attrId, attrType, sid);
                }
                else
                {
                    ReadInNonStringAttr(io, attrName, attrId, attrType, sid);
                }
                H5Sclose(sid);
                H5Aclose(attrId);
            }
        }
    }
}

void HDF5Common::ReadNativeAttrToIO(core::IO &io, hid_t datasetId,
                                    std::string const &pathFromRoot)
{
    hsize_t numAttrs;
    // herr_t ret = H5Gget_num_objs(m_FileId, &numObj);
    H5O_info_t oinfo;
    herr_t ret = H5Oget_info(datasetId, &oinfo);

    if (ret >= 0)
    {
        numAttrs = oinfo.num_attrs;

        if (numAttrs <= 0)
        {
            return; // warning: reading attrs at every var can be very time
                    // consuimg
        }
        int k = 0;
        const int MAX_ATTR_NAME_SIZE = 100;
        for (k = 0; k < numAttrs; k++)
        {
            char attrName[MAX_ATTR_NAME_SIZE];
            ret = (herr_t)H5Aget_name_by_idx(
                datasetId, ".", H5_INDEX_CRT_ORDER, H5_ITER_DEC, (hsize_t)k,
                attrName, (size_t)MAX_ATTR_NAME_SIZE, H5P_DEFAULT);
            if (ret >= 0)
            {
                hid_t attrId = H5Aopen(datasetId, attrName, H5P_DEFAULT);
                if (attrId < 0)
                {
                    continue;
                }
                if (ATTRNAME_GIVEN_ADIOSNAME.compare(attrName) == 0)
                {
                    continue;
                }

                hid_t sid = H5Aget_space(attrId);
                H5S_class_t stype = H5Sget_simple_extent_type(sid);

                hid_t attrType = H5Aget_type(attrId);
                bool isString = (H5Tget_class(attrType) == H5T_STRING);

                std::string attrNameInAdios = pathFromRoot + "/" + attrName;
                if (isString)
                {
                    ReadInStringAttr(io, attrNameInAdios, attrId, attrType,
                                     sid);
                }
                else
                {
                    ReadInNonStringAttr(io, attrNameInAdios, attrId, attrType,
                                        sid);
                }
                H5Sclose(sid);
                H5Aclose(attrId);
            }
        }
    }
}

void HDF5Common::StaticGetAdiosStepString(std::string &stepName, int ts)
{
    stepName = "/Step" + std::to_string(ts);
}

#define declare_template_instantiation(T)                                      \
    template void HDF5Common::Write(core::Variable<T> &, const T *);

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios
