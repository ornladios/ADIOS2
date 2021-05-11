/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDaos.cpp file I/O using Daos I/O library
 *
 */
#include "FileDaos.h"

#include <cstdio>      // remove
#include <cstring>     // strerror
#include <errno.h>     // errno
#include <fcntl.h>     // open
#include <stddef.h>    // write output
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

#include <daos.h>
#include <daos_fs.h>

namespace adios2
{
namespace transport
{

FileDaos::FileDaos(helper::Comm const &comm) : Transport("File", "Daos", comm)
{
    
}

FileDaos::~FileDaos()
{
    if (m_IsOpen)
    {
      //close(m_FileDescriptor);
    }
}

void FileDaos::WaitForOpen()
{
    if (m_IsOpening)
    {
        if (m_OpenFuture.valid())
        {
            m_DAOSOpenSucceed = m_OpenFuture.get();
        }
        m_IsOpening = false;
        CheckFile("couldn't open file " + m_Name + ", in call to Daos open");
        m_IsOpen = true;
    }
}

void FileDaos::Open(const std::string &name, const Mode openMode,
                    const bool async)
{
    auto lf_AsyncOpenWrite = [&](const std::string &name) -> bool {
        
        ProfilerStart("open");
        //errno = 0;
        int rc = dfs_open(/*DFS*/ dfs_mt, /*PARENT*/ NULL, m_Name.c_str(),
			  S_IFREG | S_IWUSR, 
                          O_WRONLY | O_CREAT | O_TRUNC, /*CID*/ 0,
                          /*chunksize*/ 0, NULL, &obj);
        m_Errno = rc;
        ProfilerStop("open");
	bool DAOSOpenReturn;
	if (rc == 0)
	{
	    DAOSOpenReturn = true;
	}
	else
	{
    	    DAOSOpenReturn = false;	    
	}
        return DAOSOpenReturn;
    };

    m_Name = name;
    CheckName();
    m_OpenMode = openMode;
    int rc;
    switch (m_OpenMode)
    {

    case (Mode::Write):
        if (async)
        {
            m_IsOpening = true;
            m_OpenFuture =
                std::async(std::launch::async, lf_AsyncOpenWrite, name);
        }
        else
        {
            ProfilerStart("open");
            //errno = 0;
            //m_FileDescriptor =
            //    open(m_Name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	    rc = dfs_open(/*DFS*/ dfs_mt, /*PARENT*/ NULL, m_Name.c_str(),
			  S_IFREG | S_IWUSR,
                          O_WRONLY | O_CREAT | O_TRUNC, /*CID*/ 0,
                          /*chunksize*/ 0, NULL, &obj);
            m_Errno = rc;
            ProfilerStop("open");

        }
        break;

    case (Mode::Append):
        ProfilerStart("open");
        //errno = 0;
        // m_FileDescriptor = open(m_Name.c_str(), O_RDWR);
        //m_FileDescriptor = open(m_Name.c_str(), O_RDWR | O_CREAT, 0777);
        //lseek(m_FileDescriptor, 0, SEEK_END);
	rc = dfs_open(/*DFS*/ dfs_mt, /*PARENT*/ NULL, m_Name.c_str(),
			  S_IFREG | S_IWUSR | S_IRUSR,
                          O_RDWR | O_CREAT, /*CID*/ 0,
                          /*chunksize*/ 0, NULL, &obj);
        m_Errno = rc;
        ProfilerStop("open");
        break;

    case (Mode::Read):
        ProfilerStart("open");
        //errno = 0;
        //m_FileDescriptor = open(m_Name.c_str(), O_RDONLY);
	rc = dfs_open(/*DFS*/ dfs_mt, /*PARENT*/ NULL, m_Name.c_str(),
			  S_IFREG | S_IRUSR,
                          O_RDONLY, /*CID*/ 0, /*chunksize*/ 0, NULL, &obj);
        m_Errno = rc;
        ProfilerStop("open");
        break;

    default:
        CheckFile("unknown open mode for file " + m_Name +
                  ", in call to Daos open");
    }
    if (rc == 0)
    {
	m_DAOSOpenSucceed = true;
    }
    else
    {
	m_DAOSOpenSucceed = false;	    
    }	        

    if (!m_IsOpening)
    {
        CheckFile("couldn't open file " + m_Name + ", in call to Daos open");
        m_IsOpen = true;
    }
}

void FileDaos::Write(const char *buffer, size_t size, size_t start)
{
  auto lf_Write = [&](const char *buffer, size_t size) {
        daos_size_t io_size;
	daos_size_t written_size = 0;
        d_sg_list_t wsgl;
	d_iov_t iov;
	int rc = 0;
	
	d_iov_set(&iov, const_cast<char*>(buffer), size);
	wsgl.sg_nr = 1;
	wsgl.sg_nr_out = 1;
	wsgl.sg_iovs = &iov;
        while (written_size < size)
        {
            io_size = size-written_size;
	    wsgl.sg_iovs[0].iov_len = io_size;
            ProfilerStart("write");
            //errno = 0;

            //const auto writtenSize = write(m_FileDescriptor, buffer, size);

	    rc = dfs_write(dfs_mt, obj, &wsgl, m_GlobalOffset, NULL);
	    
	    if (rc)
	    {
                throw std::ios_base::failure(
                    "ERROR: couldn't write to file " + m_Name +
                    ", in call to Daos Write" + SysErrMsg());	        
	    }
            m_Errno = rc;
            ProfilerStop("write");	    
            /*if (writtenSize == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                throw std::ios_base::failure(
                    "ERROR: couldn't write to file " + m_Name +
                    ", in call to Daos Write" + SysErrMsg());
	    }*/

            buffer += written_size;
            written_size += io_size;
	    m_GlobalOffset += io_size;
        }
    };

    WaitForOpen();
    if (start != MaxSizeT)
    {
        m_GlobalOffset = start;
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
	    lf_Write(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Write(&buffer[position], remainder);
    }
    else
    {
        lf_Write(buffer, size);
    }
}

void FileDaos::Read(char *buffer, size_t size, size_t start)
{
    auto lf_Read = [&](char *buffer, size_t size) {
        daos_size_t request_size;
	daos_size_t got_size;
	daos_size_t read_size = 0;
        d_sg_list_t rsgl;
	d_iov_t iov;
	int rc = 0;
	
	d_iov_set(&iov, const_cast<char*>(buffer), size);
	rsgl.sg_nr = 1;
	rsgl.sg_nr_out = 1;
	rsgl.sg_iovs = &iov;       
        while (read_size < size)
        {
	    request_size = size-read_size;
	    rsgl.sg_iovs[0].iov_len = request_size;
            ProfilerStart("read");

	    rc = dfs_read(dfs_mt, obj, &rsgl, m_GlobalOffset, &got_size, NULL);

	    if (rc)
	    {
                throw std::ios_base::failure(
                    "ERROR: couldn't read from file " + m_Name +
                    ", in call to Daos Read" + SysErrMsg());	        
	    }
            m_Errno = rc;
            ProfilerStop("read");	    
	    
            buffer += read_size;
            read_size += got_size;
	    m_GlobalOffset += got_size;
        }
    };

    WaitForOpen();

    if (start != MaxSizeT)
    {
        m_GlobalOffset = start;
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
	    lf_Read(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Read(&buffer[position], remainder);
    }
    else
    {
      lf_Read(buffer, size);
    }
}

size_t FileDaos::GetSize()
{
    //struct stat fileStat;
    WaitForOpen();
    daos_size_t file_size;
    int rc = 0;
    rc = dfs_get_size(dfs_mt, obj, &file_size);
    //errno = 0;
    /*if (fstat(m_FileDescriptor, &fileStat) == -1)
    {
        m_Errno = errno;
        throw std::ios_base::failure("ERROR: couldn't get size of file " +
                                     m_Name + SysErrMsg());
				     }*/
    m_Errno = rc;
    return static_cast<size_t>(file_size);
    
}

void FileDaos::Flush() {}

void FileDaos::Close()
{
    WaitForOpen();
    ProfilerStart("close");
    //errno = 0;
    int rc;
    rc = dfs_release(obj);
    //const int status = close(m_FileDescriptor);
    m_Errno = rc;
    ProfilerStop("close");

    if (rc)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to Daos IO close" +
                                     SysErrMsg());
    }

    m_IsOpen = false;
}

void FileDaos::Delete()
{
    WaitForOpen();
    if (m_IsOpen)
    {
        Close();
    }
    //std::remove(m_Name.c_str());
    int rc;
    dfs_remove(dfs_mt, NULL, m_Name.c_str(), true, NULL);
}

void FileDaos::CheckFile(const std::string hint) const
{
    if (!m_DAOSOpenSucceed)
    {
        throw std::ios_base::failure("ERROR: " + hint + SysErrMsg());
    }
}

std::string FileDaos::SysErrMsg() const
{
    std::string desc(d_errdesc(m_Errno));
    return std::string(": errno = " + std::to_string(m_Errno) + ": " + desc);
}

void FileDaos::SeekToEnd()
{
    m_GlobalOffset = GetSize();
}

void FileDaos::SeekToBegin()
{
    m_GlobalOffset = 0;
}

} // end namespace transport
} // end namespace adios2
