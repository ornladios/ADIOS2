/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Stream.h : higher-level fstream-like API
 *
 *  Created on: Jan 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_STREAM_H_
#define ADIOS2_CORE_STREAM_H_

#include <memory> //std::shared_ptr

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"

namespace adios2
{

/**
 * Private implementation of high-level APIs
 */
class Stream
{

public:
    bool m_IsOpen = false;

    StepStatus m_Status = StepStatus::NotReady;

    /** reference to IO with name m_Name inside m_ADIOS */
    IO *m_IO = nullptr;
    /** reference to Engine with name m_Name inside m_ADIOS */
    Engine *m_Engine = nullptr;

    Stream(const std::string &name, const Mode mode, MPI_Comm comm,
           const std::string engineType = "BPFile",
           const Params &parameters = Params(),
           const vParams &transportParameters = vParams(),
           const std::string hostLanguage = "C++");

    Stream(const std::string &name, const Mode mode, MPI_Comm comm,
           const std::string configFile, const std::string ioInConfigFile,
           const std::string hostLanguage = "C++");

    Stream(const std::string &name, const Mode mode,
           const std::string engineType = "BPFile",
           const Params &parameters = Params(),
           const vParams &transportParameters = vParams(),
           const std::string hostLanguage = "C++");

    Stream(const std::string &name, const Mode mode,
           const std::string configFile, const std::string ioInConfigFile,
           const std::string hostLanguage = "C++");

    Stream(const std::string hostLanguage = "C++");

    void Open(const std::string &name, const Mode mode, MPI_Comm comm,
              const std::string engineType = "BPFile",
              const Params &parameters = Params(),
              const vParams &transportParameters = vParams());

    void Open(const std::string &name, const Mode mode, MPI_Comm comm,
              const std::string configFile, const std::string ioInConfigFile);

    void Open(const std::string &name, const Mode mode,
              const std::string engineType = "BPFile",
              const Params &parameters = Params(),
              const vParams &transportParameters = vParams());

    void Open(const std::string &name, const Mode mode,
              const std::string configFile, const std::string ioInConfigFile);

    ~Stream() = default;

    template <class T>
    void Write(const std::string &name, const T *values,
               const Dims &shape = Dims{}, const Dims &start = Dims{},
               const Dims &count = Dims{}, const bool endStep = false);

    template <class T>
    void Write(const std::string &name, const T &value,
               const bool endStep = false);

    template <class T>
    void Read(const std::string &name, T *values, const bool endStep = false);

    template <class T>
    void Read(const std::string &name, T *values, const Box<size_t> &step);

    template <class T>
    void Read(const std::string &name, T *values, const Box<Dims> &selection,
              const bool endStep = false);

    template <class T>
    void Read(const std::string &name, T *values, const Box<Dims> &selection,
              const Box<size_t> &stepSelection);

    template <class T>
    std::vector<T> Read(const std::string &name, const bool endStep = false);

    template <class T>
    std::vector<T> Read(const std::string &name, const Box<Dims> &selection,
                        const bool endStep = false);

    template <class T>
    std::vector<T> Read(const std::string &name,
                        const Box<size_t> &stepSelection);

    template <class T>
    std::vector<T> Read(const std::string &name, const Box<Dims> &selection,
                        const Box<size_t> &stepSelection);

    void Close();

private:
    /** Current language bindings calling high-level Stream API */
    const std::string m_HostLanguage = "C++";

    /** Stream, IO and Engine names  */
    std::string m_Name;

    /** internal ADIOS object owned by Stream */
    std::shared_ptr<ADIOS> m_ADIOS;

    void ThrowIfNotOpen(const std::string hint) const;
    void ThrowIfOpen(const std::string hint) const;

    template <class T>
    std::vector<T> GetCommon(Variable<T> &variable, const bool endStep);

    template <class T>
    void GetPCommon(Variable<T> &variable, T *values, const bool endStep);

    template <class T>
    void CheckPCommon(const std::string &name, const T *values) const;
};

// Explicit declaration of the public template methods
#define declare_template_instantiation(T)                                      \
    extern template void Stream::Write<T>(const std::string &, const T *,      \
                                          const Dims &, const Dims &,          \
                                          const Dims &, const bool);           \
                                                                               \
    extern template void Stream::Write<T>(const std::string &, const T &,      \
                                          const bool);                         \
                                                                               \
    extern template void Stream::Read<T>(const std::string &, T *,             \
                                         const bool);                          \
                                                                               \
    extern template void Stream::Read<T>(const std::string &, T *,             \
                                         const Box<size_t> &);                 \
                                                                               \
    extern template void Stream::Read<T>(const std::string &, T *,             \
                                         const Box<Dims> &, const bool);       \
                                                                               \
    extern template void Stream::Read<T>(                                      \
        const std::string &, T *, const Box<Dims> &, const Box<size_t> &);     \
                                                                               \
    extern template std::vector<T> Stream::Read<T>(const std::string &,        \
                                                   const bool);                \
                                                                               \
    extern template std::vector<T> Stream::Read(const std::string &,           \
                                                const Box<size_t> &);          \
                                                                               \
    extern template std::vector<T> Stream::Read<T>(                            \
        const std::string &, const Box<Dims> &, const Box<size_t> &);          \
                                                                               \
    extern template std::vector<T> Stream::Read<T>(                            \
        const std::string &, const Box<Dims> &, const bool);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_CORE_STREAM_H_ */
