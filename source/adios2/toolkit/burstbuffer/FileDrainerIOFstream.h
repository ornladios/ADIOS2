/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainerFstream.h
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#ifndef ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_FSTREAM_H_
#define ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_FSTREAM_H_

#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <streambuf>
#include <string>

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace burstbuffer
{

typedef std::shared_ptr<std::ifstream> InputFile;
typedef std::shared_ptr<std::ofstream> OutputFile;

class FileDrainerIO
{
public:
    FileDrainerIO() = default;

    virtual ~FileDrainerIO() = default;

    /** turn on verbosity. set rank to differentiate between the output of
     * processes */
    void SetVerbose(int verboseLevel, int rank);

protected:
    /** rank of process just for stdout/stderr messages */
    int m_Rank = 0;
    int m_Verbose = 0;
    static const int errorState = -1;

    /** instead for Open, use this function */
    InputFile GetFileForRead(const std::string &path);
    OutputFile GetFileForWrite(const std::string &path, bool append = false);

    /** return true if the File is usable (no previous errors) */
    bool Good(InputFile &f);
    bool Good(OutputFile &f);

    void CloseAll();

    void Seek(InputFile &f, size_t offset, const std::string &path);
    void Seek(OutputFile &f, size_t offset, const std::string &path);
    void SeekEnd(OutputFile &f);

    /** Read from file. Return a pair of
     *  - number of bytes written
     *  - time spent in waiting for file to be actually written to disk for this
     * read to succeed.
     */
    std::pair<size_t, double> Read(InputFile &f, size_t count, char *buffer,
                                   const std::string &path);
    size_t Write(OutputFile &f, size_t count, const char *buffer,
                 const std::string &path);

    int FileSync(OutputFile &f);

    void Delete(OutputFile &f, const std::string &path);

private:
    typedef std::map<std::string, InputFile> InputFileMap;
    typedef std::map<std::string, OutputFile> OutputFileMap;
    InputFileMap m_InputFileMap;
    OutputFileMap m_OutputFileMap;
    void Open(InputFile &f, const std::string &path);
    void Close(InputFile &f);
    void Open(OutputFile &f, const std::string &path, bool append);
    void Close(OutputFile &f);
    size_t GetFileSize(InputFile &f);
};

} // end namespace burstbuffer
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_BURSTBUFFER_FILEDRAINER_FSTREAM_H_ */
