/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.h
 *
 *  Created on: Feb 21, 2017
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_

#include "DataManCommon.h"

namespace adios2
{
namespace core
{
namespace engine
{

class DataManReader : public DataManCommon
{

public:
    DataManReader(IO &io, const std::string &name, const Mode mode,
                  helper::Comm comm);
    virtual ~DataManReader();
    StepStatus BeginStep(StepMode stepMode, const float timeoutSeconds) final;
    size_t CurrentStep() const final;
    void PerformGets() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    bool m_ProvideLatest = true;
    bool m_InitFailed = false;
    size_t m_FinalStep = std::numeric_limits<size_t>::max();
    int m_TotalWriters;
    adios2::zmq::ZmqReqRep m_ZmqRequester;
    std::vector<std::string> m_DataAddresses;
    std::vector<std::string> m_ControlAddresses;
    std::vector<std::shared_ptr<adios2::zmq::ZmqPubSub>> m_ZmqSubscriberVec;
    format::DmvVecPtr m_CurrentStepMetadata;
    std::thread m_SubscriberThread;
    void SubscriberThread();
    void DoClose(const int transportIndex = -1) final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    DoAllStepsBlocksInfo(const Variable<T> &variable) const final;             \
    std::vector<typename Variable<T>::Info> DoBlocksInfo(                      \
        const Variable<T> &variable, const size_t step) const final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <typename T>
    void AccumulateMinMax(T &min, T &max, const std::vector<char> &minVec,
                          const std::vector<char> &maxVec) const;

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    template <typename T>
    std::map<size_t, std::vector<typename Variable<T>::Info>>
    AllStepsBlocksInfoCommon(const Variable<T> &variable) const;

    template <typename T>
    std::vector<typename Variable<T>::Info>
    BlocksInfoCommon(const Variable<T> &variable, const size_t step) const;

    template <typename T>
    void CheckIOVariable(const std::string &name, const Dims &shape,
                         const Dims &start, const Dims &count);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_ */
