/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignReader.h
 * An empty skeleton engine from which any engine can be built
 *
 *  Created on: May 15, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNREADER_H_
#define ADIOS2_ENGINE_CAMPAIGNREADER_H_

#include "CampaignData.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/remote/Remote.h"

#include <regex>

namespace adios2
{
namespace core
{
namespace engine
{

class CampaignReader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param comm
     * @param method
     * @param hostLanguage
     */
    CampaignReader(IO &adios, const std::string &name, const Mode mode, helper::Comm comm);

    ~CampaignReader();
    StepStatus BeginStep(StepMode mode = StepMode::Read, const float timeoutSeconds = -1.0) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

    MinVarInfo *MinBlocksInfo(const VariableBase &, const size_t Step) const;
    bool VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const;
    bool VariableMinMax(const VariableBase &, const size_t Step, MinMaxStruct &MinMax);
    std::string VariableExprStr(const VariableBase &Var);

private:
    UserOptions::Campaign m_Options;
    std::vector<std::string> m_IncludePatterns;  // reg.expr. include datasets
    std::vector<std::regex> m_IncludePatternsRe; // reg.expr. include datasets, compiled
    std::vector<std::string> m_ExcludePatterns;  // reg.expr. exclude datasets
    std::vector<std::regex> m_ExcludePatternsRe; // reg.expr. exclude datasets, compiled
    int m_ReaderRank;                            // my rank in the readers' comm

    int m_CurrentStep = 0;

    // EndStep must call PerformGets if necessary
    bool m_NeedPerformGets = false;

    std::vector<adios2::core::IO *> m_IOs;
    std::vector<adios2::core::Engine *> m_Engines;

    // variables coming from individual engines
    struct VarInternalInfo
    {
        void *originalVar; // Variable<T> in the actual IO
        size_t ioIdx;      // actual IO in m_IOs
        size_t engineIdx;  // actual engine in m_Engines
        VarInternalInfo(void *p, size_t i, size_t e) : originalVar(p), ioIdx(i), engineIdx(e) {}
    };
    std::unordered_map<std::string, VarInternalInfo> m_VarInternalInfo;

    // variables handled by Campaign Reader itself
    struct CampaignVarInternalInfo
    {
        void *originalVar; // Variable<T> in m_IO
        size_t dsIdx;      // in m_CampaignData.datasets
        size_t repIdx;     // in m_CampaignData.datasets.replicas
        std::string path;  // local file path, empty for in-DB data
        CampaignVarInternalInfo(void *p, size_t i, size_t j) : originalVar(p), dsIdx(i), repIdx(j)
        {
        }
        CampaignVarInternalInfo(void *p, size_t i, size_t j, const std::string &path)
        : originalVar(p), dsIdx(i), repIdx(j), path(path)
        {
        }
    };
    std::unordered_map<std::string, CampaignVarInternalInfo> m_CampaignVarInternalInfo;

    void Init() final; ///< called from constructor, gets the selected Skeleton
                       /// transport method from settings
    void ReadConfig(std::string path);
    void InitParameters() final;
    void InitTransports() final;
    void DoClose(const int transportIndex = -1);

#define declare_type(T)                                                                            \
    void DoGetSync(Variable<T> &, T *) final;                                                      \
    void DoGetDeferred(Variable<T> &, T *) final;                                                  \
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>> DoAllStepsBlocksInfo(              \
        const Variable<T> &variable) const final;                                                  \
                                                                                                   \
    std::vector<std::vector<typename Variable<T>::BPInfo>> DoAllRelativeStepsBlocksInfo(           \
        const Variable<T> &) const final;                                                          \
                                                                                                   \
    std::vector<typename Variable<T>::BPInfo> DoBlocksInfo(const Variable<T> &variable,            \
                                                           const size_t step) const final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    /* Campaign Reader own implementation of these functions */
    template <class T>
    void GetSyncTCC(Variable<T> &variable, T *data);
    template <class T>
    void GetDeferredTCC(Variable<T> &variable, T *data);
    template <class T>
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>
    AllStepsBlocksInfoTCC(const Variable<T> &variable) const;
    template <class T>
    std::vector<std::vector<typename Variable<T>::BPInfo>>
    AllRelativeStepsBlocksInfoTCC(const Variable<T> &variable) const;
    template <class T>
    std::vector<typename Variable<T>::BPInfo> BlocksInfoTCC(const Variable<T> &variable,
                                                            const size_t step) const;

    std::string SaveRemoteMD(size_t dsIdx, size_t repIdx, adios2::core::IO &io);
    void CreateDatasetAttributes(const std::string type, const std::string &name,
                                 const size_t dsIdx, const size_t repIdx,
                                 const std::string localPath);
    void CreateTextVariable(const std::string &name, const size_t len, const size_t dsIdx,
                            const size_t repIdx, const std::string localPath = "");
    void CreateImageVariable(const std::string &name, const size_t len, const size_t dsIdx,
                             const size_t repIdx, const std::string localPath = "");

    void GetVariableFromDB(std::string name, size_t dsIdx, size_t repIdx, DataType type,
                           void *data);
    void OpenDatasetWithADIOS(std::string prefixName, FileFormat format, adios2::core::IO &io,
                              std::string &localPath);

    /**
     * Called if destructor is called on an open engine.  Should warn or take
     * any non-complex measure that might help recover.
     */
    void DestructorClose(bool Verbose) noexcept final;

    /**
     * Create a new variable with name `name` in `io`
     * based on an existing variable.
     */
    template <class T>
    Variable<T> DuplicateVariable(Variable<T> *variable, IO &io, std::string &name,
                                  VarInternalInfo &vii);

    /**
     * Create a new attribute with name `name` in `io`
     * based on an existing attribute.
     */
    template <class T>
    Attribute<T> DuplicateAttribute(Attribute<T> *attribute, IO &io, std::string &name);

    /**
     * Find the actual engine/variable with name `name` in `io`
     * from the campaign variable.
     * Returns pair of variable and engine, or nullptr,nullptr if not found
     */
    template <class T>
    std::pair<Variable<T> *, Engine *> FindActualVariable(Variable<T> &variable);

    /**
     * Copy the internals of campaign variable to actual engine's variable before executing
     * operations on it
     */
    template <class T>
    Variable<T> *CopyPropertiesToActualVariable(Variable<T> &campaignVariable,
                                                Variable<T> *actualVariable);

    /** Block info function for string variables directly owned by Campaign Reader engine */
    std::vector<typename core::Variable<std::string>::BPInfo>
    BlocksInfoCommon(const core::Variable<std::string> &variable,
                     const std::vector<size_t> &blocksIndexOffsets) const;

    CampaignData m_CampaignData;
    std::unique_ptr<Remote> m_ConnectionManager = nullptr; // for reading keys from conn.manager

    // for reading individual remote file
    void ReadRemoteFile(const std::string &remoteHost, const std::string &remotePath,
                        const size_t size, void *data);

    // check if name is included and not excluded by patterns
    bool Matches(const std::string &dsname);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGNREADER_H_ */
