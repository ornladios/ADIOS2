/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FilePy.h
 *
 *  Created on: Mar 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_FILEPY_H_
#define ADIOS2_BINDINGS_PYTHON_FILEPY_H_

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/highlevelapi/fstream/Stream.h"

#include <pybind11/numpy.h>

namespace adios2
{

class FilePy
{
public:
    const std::string m_Name;
    const std::string m_Mode;

    FilePy(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string engineType = "BPFile",
           const Params &parameters = Params(),
           const vParams &transportParameters = vParams());

    FilePy(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string configFile, const std::string ioInConfigFile);

    FilePy(const std::string &name, const std::string mode,
           const std::string engineType = "BPFile",
           const Params &parameters = Params(),
           const vParams &transportParameters = vParams());

    FilePy(const std::string &name, const std::string mode,
           const std::string configFile, const std::string ioInConfigFile);

    FilePy(const FilePy &) = delete;

    FilePy(FilePy &&) = default;

    ~FilePy() = default;

    bool eof() const;

    void Write(const std::string &name, const pybind11::array &array,
               const Dims &shape, const Dims &start, const Dims &count,
               const bool endl = false);

    void Write(const std::string &name, const pybind11::array &array,
               const bool endl = false);

    void Write(const std::string &name, const std::string &stringValue,
               const bool endl = false);

    std::string ReadString(const std::string &name, const bool endl = false);

    std::string ReadString(const std::string &name, const size_t step);

    pybind11::array Read(const std::string &name, const bool endl = false);

    pybind11::array Read(const std::string &name, const size_t step);

    pybind11::array Read(const std::string &name, const Dims &selectionStart,
                         const Dims &selectionCount, const bool endl = false);

    pybind11::array Read(const std::string &name, const Dims &selectionStart,
                         const Dims &selectionCount,
                         const size_t stepSelectionStart,
                         const size_t stepSelectionCount);

    void Close();

    bool IsClosed() const noexcept;

private:
    std::shared_ptr<Stream> m_Stream;
    bool m_IsClosed = true;
};
}

#endif /* ADIOS2_BINDINGS_PYTHON_FILEPY_H_ */
