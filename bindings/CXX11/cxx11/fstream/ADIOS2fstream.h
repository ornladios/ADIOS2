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
    /** Available open modes for adios2::fstream constructor or open calls */
    enum openmode
    {
        out, //!< write
        in,  //!< read
        app  //!< append, not yet supported
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
     * @throws std::invalid_argument (user input error) or std::runtime_error
     * (system error)
     */
    fstream(const std::string &name, adios2::fstream::openmode mode,
            MPI_Comm comm, const std::string engineType = "BPFile");

    /**
     * High-level API MPI constructor, based on C++11 fstream. Allows for
     * runtime config file.
     * @param name stream name
     * @param mode fstream::in (Read), fstream::out (Write), fstream::app
     * (Append)
     * @param comm MPI communicator establishing domain for fstream
     * @param configFile adios2 runtime configuration file
     * @param ioInConfigFile specific io name in configFile
     * @throws std::invalid_argument (user input error) or
     * std::runtime_error
     * (system error)
     */
    fstream(const std::string &name, const adios2::fstream::openmode mode,
            MPI_Comm comm, const std::string &configFile,
            const std::string ioInConfigFile);
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
    fstream(const std::string &name, const adios2::fstream::openmode mode,
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
    fstream(const std::string &name, const adios2::fstream::openmode mode,
            const std::string &configFile, const std::string ioInConfigFile);
#endif
    /** Empty constructor, allows the use of open later in the code */
    fstream() = default;

    /** Using RAII STL containers only */
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
     * @param mode adios2::fstream::in (Read), adios2::fstream::out (Write),
     * adios2::fstream::app (Append)
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
     * @brief Define attribute inside fstream or for a variable after write.
     * Single value input version
     * @param name unique attribute identifier IO object or for a Variable if
     * variableName is not empty (associated to a variable)
     * @param value single data value
     * @param variableName default is empty, if not empty attributes is
     * associated to a variable after a write
     * @param separator default is "/", hierarchy between variable name and
     * attribute, e.g. variableName/attribute1, variableName::attribute1. Not
     * used if variableName is empty.
     */
    template <class T>
    void write_attribute(const std::string &name, const T &value,
                         const std::string &variableName = "",
                         const std::string separator = "/");

    /**
     * @brief Define attribute inside fstream or for a variable after write.
     * Array input version
     * @param name unique attribute identifier IO object or for a Variable if
     * variableName is not empty (associated to a variable)
     * @param data pointer to user data
     * @param size number of data elements
     * @param variableName default is empty, if not empty attributes is
     * associated to a variable after a write
     * @param separator default is "/", hierarchy between variable name and
     * attribute, e.g. variableName/attribute1, variableName::attribute1. Not
     * used if variableName is empty.
     */
    template <class T>
    void write_attribute(const std::string &name, const T *data,
                         const size_t elements,
                         const std::string &variableName = "",
                         const std::string separator = "/");

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
               const adios2::Dims &shape = adios2::Dims(),
               const adios2::Dims &start = adios2::Dims(),
               const adios2::Dims &count = adios2::Dims(),
               const bool endl = false);

    /**
     * Write a self-describing single-value variable
     * @param name variable name
     * @param value variable data value (can be r-value)
     * @param endl similar to std::endl, end current step and flush (default).
     * Use adios2::endl for true.
     * @exception std::invalid_argument (user input error) or
     * std::runtime_error (system error)
     */
    template <class T>
    void write(const std::string &name, const T &value,
               const bool endl = false);

    /**
     * Reads into a pre-allocated pointer a selection piece in dimension. When
     * used with adios2::getstep reads current step
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
     */
    template <class T>
    void read(const std::string &name, T *values);

    /**
     * Reads a value. When used with adios2::getstep reads current step value
     * @param name variable name
     * @param value output value, if variable is not found (name and type don't
     * match) the returned value address becomes nullptr
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    void read(const std::string &name, T &value);

    /**
     * Read accessing steps in random access mode. Not be used with
     * adios2::getstep as it throw an exception when reading in stepping mode.
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
     * @param stepsStart variable initial step (relative to the variable first
     * appearance, not absolute step in stream)
     * @param stepsCount variable number of steps form step_start, don't have to
     * be contiguous, necessarily
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    void read(const std::string &name, T *values, const size_t stepsStart,
              const size_t stepsCount = 1);

    /**
     * Reads into a single value for a single step. Not be used with
     * adios2::getstep as it throws an exception when reading in stepping mode.
     * @param name variable name
     * @param value filled with value,
     * if variable is not found (name, type and step don't match) the returned
     * value address becomes nullptr
     * @param step selected single step
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    void read(const std::string &name, T &value, const size_t step);

    /**
     * Reads into a pre-allocated pointer a selection piece in dimension. When
     * used with adios2::getstep reads current step
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes nullptr
     * @param start variable local offset selection
     * @param count variable local dimension selection from start
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    void read(const std::string &name, T *values, const adios2::Dims &start,
              const adios2::Dims &count);

    /**
     * Reads into a pre-allocated pointer a selection piece in dimensions and
     * steps. Not be used with adios2::getstep as it throws an exception when
     * reading in stepping mode.
     * @param name variable name
     * @param values pre-allocated pointer to hold read values, if variable is
     * not found (name and type don't match) it becomes a nullptr
     * @param start variable local offset selection
     * @param count variable local dimension selection from start
     * @param stepsStart variable initial step (relative to the variable first
     * appearance, not absolute step in stream)
     * @param stepsCount variable number of steps form step_start, don't have to
     * be necessarily contiguous
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    void read(const std::string &name, T *values, const adios2::Dims &start,
              const adios2::Dims &count, const size_t stepsStart,
              const size_t stepsCount);

    /**
     * Reads entire variable for current step (streaming mode: step by step)
     * @param name variable name
     * @return values of variable name for current step. Single values will have
     * a size=1 vector
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    std::vector<T> read(const std::string &name);

    /**
     * Return single value given name and step
     * @param name variable name
     * @param step input step
     * @return value if name and step are found
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    T read(const std::string &name, const size_t step);

    /**
     * Returns a vector with full variable dimensions for the current step
     * selection. Not be used with adios2::getstep as it throw an exception when
     * reading in stepping mode.
     * @param name variable name
     * @param stepsStart variable initial step (relative to the variable first
     * appearance, not absolute step in stream)
     * @param stepsCount variable number of steps form step_start, don't have to
     * be contiguous, necessarily
     * @return values of variable name for current step, empty if exception is
     * thrown
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    std::vector<T> read(const std::string &name, const size_t stepsStart,
                        const size_t stepsCount);

    /**
     * Reads a selection piece in dimension for current step (streaming mode:
     * step by step)
     * @param name variable name
     * @param start variable local offset selection
     * @param count variable local dimension selection from start
     * @return values of variable name for current step, empty if exception is
     * thrown
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    std::vector<T> read(const std::string &name, const Dims &start,
                        const Dims &count);

    /**
     * Reads a selection piece in dimension and a selection piece in steps
     * (non-streaming mode). Not be used with adios2::getstep as it throw an
     * exception when reading in stepping mode.
     * @param name variable name
     * @param start variable local offset selection
     * @param count variable local dimension selection from start
     * @param stepsStart variable initial step (relative to the variable first
     * appearance, not absolute step in stream)
     * @param stepsCount variable number of steps form step_start, don't have to
     * be contiguous, necessarily
     * @return variable values, empty if exception is thrown
     * @exception throws exception if variable name, dimensions or step not
     * found
     */
    template <class T>
    std::vector<T> read(const std::string &name, const Dims &start,
                        const Dims &count, const size_t stepsStart,
                        const size_t stepsCount);

    template <class T>
    std::vector<T> read_attribute(const std::string &name,
                                  const std::string &variableName = "",
                                  const std::string separator = "/");

    /** close current stream becoming inaccessible */
    void close();

    /**
     * Gets step from stream
     * Based on std::getline, enables reading on a step-by-step basis in a while
     * or for loop. Read mode only
     * @param stream input stream containing steps
     * @param step output object current step, adios2::fstep in an alias to
     * adios2::fstream with scope narrowed to one step
     * @return true: step is valid, false: step is invalid (end of stream).
     */
    friend bool getstep(adios2::fstream &stream, adios2::fstep &step);

    /**
     * Return current step when getstep is called in a loop, read mode only
     * @return current step
     */
    size_t currentstep() const noexcept;

protected:
    std::shared_ptr<core::Stream> m_Stream;

    adios2::Mode ToMode(const openmode mode) const noexcept;

private:
    fstream(fstream &stream) = default;

    void CheckOpen(const std::string &name) const;
};

#define declare_template_instantiation(T)                                      \
    extern template void fstream::write_attribute<T>(                          \
        const std::string &, const T &, const std::string &,                   \
        const std::string);                                                    \
                                                                               \
    extern template void fstream::write_attribute<T>(                          \
        const std::string &, const T *, const size_t, const std::string &,     \
        const std::string);                                                    \
                                                                               \
    extern template std::vector<T> fstream::read_attribute<T>(                 \
        const std::string &, const std::string &, const std::string);

ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

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
