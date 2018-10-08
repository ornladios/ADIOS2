/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS2fstream.h
 *
 *  Created on: Mar 5, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_H_
#define ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_H_

#include <memory> //std::shared_ptr

#include "adios2/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#endif

#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{
class fstream;
using fstep = fstream;
bool getstep(adios2::fstream &stream, adios2::fstep &step);
}

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
namespace core
{
class Stream;
}
/// \endcond

class fstream
{
public:
    enum openmode
    {
        out,
        in,
        app
    };

#ifdef ADIOS2_HAVE_MPI
    /**
     * High-level API MPI constructor, based on C++11 fstream. Allows for
     * passing parameters in source code.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param comm MPI communicator establishing domain for fstream
     * @param engineType available adios2 engine
     * @exception std::invalid_argument (user input error) or std::runtime_error
     * (system error)
     */
    fstream(const std::string &name, const openmode mode, MPI_Comm comm,
            const std::string engineType = "BPFile");

    /**
     * High-level API MPI constructor, based on C++11 fstream. Allows for
     * runtime config file.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param comm MPI communicator establishing domain for fstream
     * @param configFile adios2 runtime configuration file
     * @param ioInConfigFile specific io name in configFile
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error
     * (system error)
     */
    fstream(const std::string &name, const openmode mode, MPI_Comm comm,
            const std::string &configFile, const std::string ioInConfigFile);
#else
    /**
     * High-level API non-MPI constructor, based on C++11 fstream. Allows for
     * passing parameters in source code.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param engineType available adios2 engine
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    fstream(const std::string &name, const openmode mode,
            const std::string engineType = "BPFile");

    /**
     * High-level API MPI constructor, based on C++11 fstream. Allows for
     * runtime config file.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param configFile adios2 runtime configuration file
     * @param ioInConfigFile specific io name in configFile
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    fstream(const std::string &name, const openmode mode,
            const std::string &configFile, const std::string ioInConfigFile);
#endif
    /** Empty constructor, allows the use of open later in the code */
    fstream() = default;

    ~fstream() = default;

    /**
     * Checks if fstream object is valid
     */
    explicit operator bool() const noexcept;

#ifdef ADIOS2_HAVE_MPI
    /**
     * High-level API MPI open, based on C++11 fstream. Allows for
     * passing parameters in source code. Used after empty constructor.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param comm MPI communicator establishing domain for fstream
     * @param engineType available adios2 engine
     * @exception std::invalid_argument (user input error) or std::runtime_error
     * (system error)
     */
    void open(const std::string &name, const openmode mode, MPI_Comm comm,
              const std::string engineType = "BPFile");

    /**
     * High-level API MPI constructor, based on C++11 fstream. Allows for
     * runtime config file. Used after empty constructor.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param comm MPI communicator establishing domain for fstream
     * @param configFile adios2 runtime configuration file
     * @param ioInConfigFile specific io name in configFile
     * @exception std::invalid_argument (user input error) or std::runtime_error
     * (system error)
     */
    void open(const std::string &name, const openmode mode, MPI_Comm comm,
              const std::string configFile, const std::string ioInConfigFile);
#else
    /**
     * High-level API non-MPI open, based on C++11 fstream. Allows for
     * passing parameters in source code. Used after empty constructor.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param engineType available adios2 engine
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    void open(const std::string &name, const openmode mode,
              const std::string engineType = "BPFile");

    /**
     * High-level API non-MPI constructor, based on C++11 fstream. Allows for
     * runtime config file. Used after empty constructor.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param configFile adios2 runtime configuration file
     * @param ioInConfigFile specific io name in configFile
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    void open(const std::string &name, const openmode mode,
              const std::string configFile, const std::string ioInConfigFile);
#endif
    /**
     * writes a self-describing array variable
     * @param name variable name
     * @param values variable data values
     * @param shape variable global MPI dimensions. Pass empty for local
     * variables.
     * @param start variable offset for current MPI rank. Pass empty for local
     * variables.
     * @param count variable dimension for current MPI rank. Local variables
     * only have count.
     * @param endl similar to std::endl, end current step and flush (default).
     * Use adios2::endl for true.
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    template <class T>
    void write(const std::string &name, const T *values,
               const Dims &shape = Dims{}, const Dims &start = Dims{},
               const Dims &count = Dims{}, const bool endl = false);

    /**
     * writes a self-describing single-value variable
     * @param name variable name
     * @param values variable data value (can be r-value)
     * @param endl similar to std::endl, end current step and flush (default).
     * Use adios2::endl for true.
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    template <class T>
    void write(const std::string &name, const T &value,
               const bool endl = false);

    /**
     * Reads into a pre-allocated values pointer for current step (streaming
     * mode: step by step)
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
     * @param endl end current step and moves forward to the next step
     */
    template <class T>
    void read(const std::string &name, T *values);

    /**
     * Reads into a single value for current step (streaming
     * mode: step by step)
     * @param name variable name
     * @param value filled with value,
     * if variable is not found (name and type don't match) the returned address
     * becomes nullptr
     * @param endl true: end current step and moves forward to the next step
     */
    template <class T>
    void read(const std::string &name, T &value);

    /**
     * Reads into an array variable for a range of steps
     * @param name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
      * @param stepSelectionStart initial step point
     * @param stepSelectionCount number of steps from stepSelectionStart
     */
    template <class T>
    void read(const std::string &name, T *values,
              const size_t stepSelectionStart,
              const size_t stepSelectionCount = 1);

    /**
     * Reads into a single value for a range of steps
     * @param name variable name
     * @param value filled with value,
     * if variable is not found (name, type and step don't match) the returned
     * address becomes nullptr
     * @param step selected single step
     */
    template <class T>
    void read(const std::string &name, T &value, const size_t step);

    /**
     * Reads into a pre-allocated pointer a selection piece in dimension for
     * current step (streaming mode: step by step)
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
     * @param selectionStart dimension box selection start point
     * @param selectionCount dimension box selection count (length) per
     * direction
     * @param endl true: end current step and moves forward to the next step
     */
    template <class T>
    void read(const std::string &name, T *values, const Dims &selectionStart,
              const Dims &selectionCount);

    /**
     * Reads into a pre-allocated pointer a selection piece in dimensions and
     * steps
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
     * @param selectionStart dimension box selection start point
     * @param selectionCount dimension box selection count (length) per
     * direction
     * @param stepSelectionStart initial step point
     * @param stepSelectionCount number of steps from stepSelectionStart
     */
    template <class T>
    void read(const std::string &name, T *values, const Dims &selectionStart,
              const Dims &selectionCount, const size_t stepSelectionStart,
              const size_t stepSelectionCount);

    /**
     * Reads entire variable for current step (streaming mode: step by step)
     * @param name variable name
     * @param endl true: end current step and moves forward to the next step
     * @return values of variable name for current step. Single values will have
     * a size=1 vector
     */
    template <class T>
    std::vector<T> read(const std::string &name);

    /**
     * Return single value given name and size
     * @param name variable name
     * @param step input step
     * @return value if name and step are found
     * @exception throws exception if variable not found for name and step
     */
    template <class T>
    T read(const std::string &name, const size_t step);

    /**
     * Returns a vector with full variable dimensions for the current step
     * selection
     * @param name variable name
     * @param stepSelectionStart initial step point
     * @param stepSelectionCount number of steps from stepSelectionStart
     * @return values of variable name for current step, empty if variable not
     * found or selections are out of bounds
     */
    template <class T>
    std::vector<T> read(const std::string &name,
                        const size_t stepSelectionStart,
                        const size_t stepSelectionCount);

    /**
     * Reads a selection piece in dimension for current step (streaming mode:
     * step by step)
     * @param name variable name
     * @param selectionStart dimension box selection start point
     * @param selectionCount dimension box selection count (length) per
     * direction
     * @param endl true: end current step and moves forward to the next step
     * @return values of variable name for current step, empty if variable not
     * found or selections are out of bounds
     */
    template <class T>
    std::vector<T> read(const std::string &name, const Dims &selectionStart,
                        const Dims &selectionCount);

    /**
     * Reads a selection piece in dimension and a selection piece in steps
     * (non-streaming mode)
     * @param name variable name
     * @param selectionStart dimension box selection start point
     * @param selectionCount dimension box selection count (length) per
     * direction
     * @param stepSelectionStart initial step point
     * @param stepSelectionCount number of steps from stepSelectionStart
     * @return values of variable name and type<T> for current selection
     */
    template <class T>
    std::vector<T> read(const std::string &name, const Dims &selectionStart,
                        const Dims &selectionCount,
                        const size_t stepSelectionStart,
                        const size_t stepSelectionCount);

    /** close current stream becoming inaccessible */
    void close();

    friend bool getstep(adios2::fstream &stream, adios2::fstep &step);

    size_t currentstep() const noexcept;

protected:
    std::shared_ptr<core::Stream> m_Stream;

    adios2::Mode ToMode(const openmode mode) const noexcept;

private:
    fstream(fstream &stream) = default;

    void CheckOpen(const std::string name) const;
};

#define declare_template_instantiation(T)                                      \
    extern template void fstream::write<T>(const std::string &, const T *,     \
                                           const Dims &, const Dims &,         \
                                           const Dims &, const bool);          \
                                                                               \
    extern template void fstream::write<T>(const std::string &, const T &,     \
                                           const bool);                        \
                                                                               \
    extern template std::vector<T> fstream::read<T>(const std::string &);      \
                                                                               \
    extern template std::vector<T> fstream::read<T>(                           \
        const std::string &, const Dims &, const Dims &);                      \
                                                                               \
    extern template std::vector<T> fstream::read<T>(                           \
        const std::string &, const Dims &, const Dims &, const size_t,         \
        const size_t);                                                         \
                                                                               \
    extern template void fstream::read<T>(const std::string &, T *);           \
                                                                               \
    extern template void fstream::read<T>(const std::string &name, T &);       \
                                                                               \
    extern template void fstream::read<T>(const std::string &name, T &,        \
                                          const size_t);                       \
                                                                               \
    extern template void fstream::read<T>(const std::string &, T *,            \
                                          const Dims &, const Dims &);         \
                                                                               \
    extern template void fstream::read<T>(const std::string &, T *,            \
                                          const size_t, const size_t);         \
                                                                               \
    extern template void fstream::read<T>(const std::string &, T *,            \
                                          const Dims &, const Dims &,          \
                                          const size_t, const size_t);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_CXX11_CXX11_FSTREAM_ADIOS2FSTREAM_H_ */
