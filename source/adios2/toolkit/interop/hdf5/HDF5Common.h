/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDF5CommonP.h
 *
 *  Created on: March 20, 2017
 *      Author: Junmin
 */

#ifndef ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_
#define ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_

#include <hdf5.h>

#include <string>

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h" // for CreateVar
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosComm.h"

#include <stdexcept> // for Intel Compiler

namespace adios2
{
namespace interop
{

typedef enum
{
    E_H5_DATASET = 0,
    E_H5_DATATYPE = 1,
    E_H5_GROUP = 2,
    E_H5_SPACE = 3,
} ADIOS_ENUM_H5;

class HDF5DatasetGuard
{
public:
    HDF5DatasetGuard(std::vector<hid_t> &chain) : m_Chain(chain) {}

    ~HDF5DatasetGuard()
    {
        if (m_Chain.size() == 0)
        {
            return;
        }

        for (int i = 0; i < m_Chain.size() - 1; i++)
        {
            H5Gclose(m_Chain[i]);
        }
        H5Dclose(m_Chain.back());
    }

private:
    std::vector<hid_t> m_Chain;
};

class HDF5TypeGuard
{
public:
    HDF5TypeGuard(hid_t key, ADIOS_ENUM_H5 type)
    {
        m_Key = key;
        m_Type = type;
        if (key < 0)
        {
            throw std::ios_base::failure("ERROR: HDF5 failure detected.");
        }
    }

    ~HDF5TypeGuard()
    {
        if (m_Type == E_H5_DATASET)
        {
            H5Dclose(m_Key);
        }
        else if (m_Type == E_H5_GROUP)
        {
            H5Gclose(m_Key);
        }
        else if (m_Type == E_H5_SPACE)
        {
            H5Sclose(m_Key);
        }
        else if (m_Type == E_H5_DATATYPE)
        {
            H5Tclose(m_Key);
        }
        else
        {
            printf(" UNABLE to close \n");
        }
    }

private:
    ADIOS_ENUM_H5 m_Type;
    hid_t m_Key;
};

class HDF5Common
{

public:
    /**
     * Unique constructor for HDF5 file
     * @param debugMode true: extra exception checks
     */
    HDF5Common(const bool debugMode);

    static const std::string ATTRNAME_NUM_STEPS;
    static const std::string ATTRNAME_GIVEN_ADIOSNAME;
    static const std::string PREFIX_BLOCKINFO;
    static const std::string PREFIX_STAT;

    static const std::string PARAMETER_COLLECTIVE;
    static const std::string PARAMETER_CHUNK_FLAG;
    static const std::string PARAMETER_CHUNK_VARS;

    void ParseParameters(core::IO &io);
    void Init(const std::string &name, helper::Comm const &comm, bool toWrite);

    template <class T>
    void Write(core::Variable<T> &variable, const T *values);

    void CreateDataset(const std::string &varName, hid_t h5Type,
                       hid_t filespaceID, std::vector<hid_t> &chain);
    bool OpenDataset(const std::string &varName, std::vector<hid_t> &chain);

    void StoreADIOSName(const std::string adiosName, hid_t dsetID);
    void ReadADIOSName(hid_t dsetID, std::string &adiosName);

    void LocateAttrParent(const std::string &attrName,
                          std::vector<std::string> &list,
                          std::vector<hid_t> &datasetChain);

    void Close();
    void Advance();

    void WriteAttrFromIO(core::IO &io);
    void ReadAttrToIO(core::IO &io);

    void ReadNativeAttrToIO(core::IO &io, hid_t datasetId,
                            std::string const &pathFromRoot);

    void SetAdiosStep(int ts);

    unsigned int GetNumAdiosSteps();
    void WriteAdiosSteps();

    void ReadVariables(unsigned int ts, core::IO &io);
    void FindVarsFromH5(core::IO &io, hid_t gid, const char *name,
                        const char *heritage, unsigned int ts);
    void ReadAllVariables(core::IO &io);

    void ReadStringScalarDataset(hid_t dataSetId, std::string &result);
    hid_t GetTypeStringScalar(const std::string &input);
    void CreateVar(core::IO &io, hid_t h5Type, std::string const &name,
                   unsigned int ts);

    template <class T>
    void AddVar(core::IO &io, std::string const &name, hid_t datasetId,
                unsigned int ts);

    template <class T>
    void AddNonStringAttribute(core::IO &io, std::string const &attrName,
                               hid_t attrId, hid_t h5Type, hsize_t arraySize);

    static void StaticGetAdiosStepString(std::string &adiosStepName, int ts);

    hid_t m_PropertyListId = -1;
    hid_t m_PropertyTxfID = -1;
    hid_t m_FileId = -1;
    hid_t m_GroupId = -1;

    hid_t m_DefH5TypeComplexDouble;
    hid_t m_DefH5TypeComplexFloat;
    hid_t m_DefH5TypeBlockStat;

    unsigned int m_CurrentAdiosStep = 0;

    void CheckWriteGroup();

    template <class T>
    hid_t GetHDF5Type(); // should this be public?

    template <class T>
    T GetADIOSType(hid_t);

    bool m_IsGeneratedByAdios = false;

private:
    void ReadInStringAttr(core::IO &io, const std::string &attrName,
                          hid_t attrId, hid_t h5Type, hid_t sid);
    void ReadInNonStringAttr(core::IO &io, const std::string &attrName,
                             hid_t attrId, hid_t h5Type, hid_t sid);
    void WriteStringAttr(core::IO &io, core::Attribute<std::string> *adiosAttr,
                         const std::string &attrName, hid_t parentID);

    template <class T>
    void WriteNonStringAttr(core::IO &io, core::Attribute<T> *adiosAttr,
                            hid_t parentID, const char *h5Name);

    const bool m_DebugMode;
    bool m_WriteMode = false;
    unsigned int m_NumAdiosSteps = 0;

    int m_CommRank = 0;
    int m_CommSize = 1;

    template <class T>
    void AddBlockInfo(const core::Variable<T> &varaible, hid_t parentId);
    template <class T>
    void AddStats(const core::Variable<T> &variable, hid_t parentId,
                  std::vector<T> &stats);

    hid_t m_ChunkPID;
    int m_ChunkDim;
    std::set<std::string> m_ChunkVarNames;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void HDF5Common::Write(core::Variable<T> &variable,        \
                                           const T *value);
ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace interop
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_INTEROP_HDF5_HDF5COMMON_H_ */
