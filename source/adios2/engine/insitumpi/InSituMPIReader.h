/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIReader.h
 * Class to exchange data using MPI between Writer and Reader
 *  partition of an application
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPIREADER_H_
#define ADIOS2_ENGINE_INSITUMPIREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/format/bp3/BP3.h"

namespace adios2
{

class InSituMPIReader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param hostLanguage
     */
    InSituMPIReader(IO &adios, const std::string &name, const Mode mode,
                    MPI_Comm mpiComm);

    ~InSituMPIReader();
    StepStatus BeginStep(StepMode mode = StepMode::NextAvailable,
                         const float timeoutSeconds = 0.f) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

private:
    MPI_Comm m_CommWorld = MPI_COMM_WORLD;
    int m_Verbosity = 0;

    int m_GlobalRank; // my rank in the global comm
    int m_ReaderRank; // my rank in the readers' comm
    int m_GlobalNproc, m_ReaderNproc;

    // Global ranks of all the writers
    std::vector<int> m_RankAllPeers;
    // Global ranks of the writers directly assigned to me
    std::vector<int> m_RankDirectPeers;

    // global rank of write root (-1 if not connected to root)
    int m_WriteRootGlobalRank = -1;
    // local rank of the reader who is connected to write root
    // the Reader Root receives the metadata from the Write root
    int m_ReaderRootRank;

    int m_CurrentStep = -1; // steps start from 0

    int m_NCallsPerformGets; // 1 and only 1 PerformGets() allowed per step

    /** Single object controlling BP buffering used only for metadata in this
     * engine */
    format::BP3Deserializer m_BP3Deserializer;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    void DoGetDeferred(Variable<T> &, T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    void DoClose(const int transportIndex = -1) final;

    void ClearMetadataBuffer();

    // BP3 format style read schedule, keeping it around in fixed schedule
    std::map<std::string, SubFileInfoMap> m_ReadScheduleMap;

    void SendReadSchedule(
        const std::map<std::string, SubFileInfoMap> &variablesSubFileInfo);

    uint64_t m_BytesReceivedInPlace = 0; // bytes that were arriving in place
    uint64_t m_BytesReceivedInTemporary = 0; // bytes that needed copy
    int Statistics(uint64_t bytesInPlace, uint64_t bytesCopied);

    // Make an async receive request for all variables
    void AsyncRecvAllVariables();

    template <class T>
    void AsyncRecvVariable(const Variable<T> &variable,
                           const SubFileInfoMap &subFileInfoMap);

    // Wait for all async receives to arrive and process them
    void ProcessReceives();

    struct OngoingReceive
    {
        const SubFileInfo *sfiPointer;
        const std::string *varNamePointer;
        std::vector<char> temporaryDataArray; // allocated in engine
        char *inPlaceDataArray;               // pointer to user data
        OngoingReceive(const SubFileInfo *p, const std::string *v)
        : sfiPointer(p), varNamePointer(v), inPlaceDataArray(nullptr){};
        OngoingReceive(const SubFileInfo *p, const std::string *v, char *ptr)
        : sfiPointer(p), varNamePointer(v), inPlaceDataArray(ptr){};
    };

    std::vector<OngoingReceive> m_OngoingReceives;
    // We need a contiguous array of MPI_Requests, so we
    // have it here separately from OnGoingReceive struct
    std::vector<MPI_Request> m_MPIRequests;
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIMPIREADER_H_ */
