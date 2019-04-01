/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11File.h
 *
 *  Created on: Mar 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_PY11FILE_H_
#define ADIOS2_BINDINGS_PYTHON_PY11FILE_H_

#include <pybind11/numpy.h>

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Stream.h"

namespace adios2
{
namespace py11
{

class File
{
public:
    const std::string m_Name;
    const std::string m_Mode;

    File(const std::string &name, const std::string mode, MPI_Comm comm,
         const std::string engineType = "BPFile");

    File(const std::string &name, const std::string mode, MPI_Comm comm,
         const std::string &configFile, const std::string ioInConfigFile);

    File(const std::string &name, const std::string mode,
         const std::string engineType = "BPFile");

    File(const std::string &name, const std::string mode,
         const std::string &configFile, const std::string ioInConfigFile);

    ~File() = default;

    void SetParameter(const std::string key, const std::string value) noexcept;

    void SetParameters(const Params &parameters) noexcept;

    size_t AddTransport(const std::string type,
                        const Params &parameters = Params());

    std::map<std::string, adios2::Params> AvailableVariables() noexcept;

    std::map<std::string, adios2::Params> AvailableAttributes() noexcept;

    void WriteAttribute(const std::string &name, const pybind11::array &array,
                        const std::string &variableName = "",
                        const std::string separator = "/",
                        const bool endstep = false);

    void WriteAttribute(const std::string &name, const std::string &stringValue,
                        const std::string &variableName = "",
                        const std::string separator = "/",
                        const bool endstep = false);

    void WriteAttribute(const std::string &name,
                        const std::vector<std::string> &stringArray,
                        const std::string &variableName = "",
                        const std::string separator = "/",
                        const bool endstep = false);

    void Write(const std::string &name, const pybind11::array &array,
               const Dims &shape, const Dims &start, const Dims &count,
               const bool endstep = false);

    void Write(const std::string &name, const pybind11::array &array,
               const bool endstep = false);

    void Write(const std::string &name, const std::string &stringValue,
               const bool endstep = false);

    bool GetStep() const;

    std::vector<std::string> ReadString(const std::string &name);

    std::vector<std::string> ReadString(const std::string &name,
                                        const size_t stepStart,
                                        const size_t stepCount);

    pybind11::array Read(const std::string &name);

    pybind11::array Read(const std::string &name, const Dims &selectionStart,
                         const Dims &selectionCount);

    pybind11::array Read(const std::string &name, const Dims &selectionStart,
                         const Dims &selectionCount,
                         const size_t stepSelectionStart,
                         const size_t stepSelectionCount);

    pybind11::array ReadAttribute(const std::string &name,
                                  const std::string &variableName = "",
                                  const std::string separator = "/");

    std::vector<std::string>
    ReadAttributeString(const std::string &name,
                        const std::string &variableName = "",
                        const std::string separator = "/");

    void EndStep();

    void Close();

    size_t CurrentStep() const;

private:
    std::shared_ptr<core::Stream> m_Stream;

    adios2::Mode ToMode(const std::string mode) const;
};

} // end namespace py11
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_PYTHON_FILEPY_H_ */
