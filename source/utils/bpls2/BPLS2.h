/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * bpls2.h
 *
 *  Created on: Oct 5, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef UTILS_BPLS2_BPLS2_H_
#define UTILS_BPLS2_BPLS2_H_

#include "utils/Utils.h"

namespace adios2
{
namespace utils
{

class BPLS2 : public Utils
{
public:
    BPLS2(int argc, char *argv[]);

    ~BPLS2() = default;

    void Run() final;

private:
    static const std::string m_HelpMessage;
    static const Params m_Options;

    std::string m_FileName;

    void ParseArguments() final;
    void ProcessParameters() const final;
    void PrintUsage() const noexcept final;
    void PrintExamples() const noexcept final;
    void SetParameters(const std::string argument, const bool isLong) final;

    void ProcessTransport() const;
};

} // end namespace utils
} // end namespace adios2

#endif /* UTILS_BPLS2_BPLS2_H_ */
