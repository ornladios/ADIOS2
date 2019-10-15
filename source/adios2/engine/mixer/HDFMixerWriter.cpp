/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * HDFMixer.h
 *
 *  Created on: Aug 16 2017
 *      Author: Junmin GU
 */

#include <iostream>

#include "HDFMixerWriter.h"
#include "adios2/helper/adiosFunctions.h"

//
// class HDFSerialWriter
//
namespace adios2
{
namespace core
{
namespace engine
{

HDFVDSWriter::HDFVDSWriter(helper::Comm const &comm, bool debugMode)
: m_SubfileComm(comm), m_VDSFile(debugMode), m_Rank(-1)
{
    m_NumSubFiles = m_SubfileComm.Size();
    m_Rank = m_SubfileComm.Rank();
}

void HDFVDSWriter::Init(const std::string &name)
{
    if (m_Rank > 0)
    {
        return;
    }

    //
    // VDS can only operate on one process. So let rank = 0 handle it
    //
    std::string h5Name = adios2::helper::AddExtension(name, ".h5");
    m_VDSFile.Init(h5Name, helper::Comm(), true);
    // m_FileName = h5Name;
    m_FileName = name;
}

void HDFVDSWriter::GetVarInfo(const VariableBase &var,
                              std::vector<hsize_t> &dimsf, int nDims,
                              std::vector<hsize_t> &start,
                              std::vector<hsize_t> &count,
                              std::vector<hsize_t> &one)
{ // interop::HDF5Common summaryFile(true);
    // std::vector<hsize_t> dimsf, start, one, count;
    // int nDims = std::max(var.m_Shape.size(), var.m_Count.size());

    for (int i = 0; i < nDims; i++)
    {
        if (var.m_Shape.size() > 0)
        {
            dimsf.push_back(var.m_Shape[i]);
        }
        else
        {
            dimsf.push_back(var.m_Count[i]);
        }
        if (var.m_Start.size() > 0)
        {
            start.push_back(var.m_Start[i]);
        }
        else
        {
            start.push_back(0);
        }
        if (var.m_Count.size() > 0)
        {
            count.push_back(var.m_Count[i]);
        }
        else if (var.m_Shape.size() > 0)
        {
            count.push_back(var.m_Shape[i]);
        }
        else
        {
            count.push_back(0);
        }
        one.push_back(1);
    }
}

void HDFVDSWriter::AddVar(const VariableBase &var, hid_t h5Type)
{
    hid_t space;
    /* Create VDS dataspace.  */
    int nDims = std::max(var.m_Shape.size(), var.m_Count.size());

    if (nDims == 0)
    {
        if (m_Rank == 0)
        {
            /*
            std::cout<<" will deal with scalar later?"<<var.m_Name<<std::endl;

            hid_t filespaceID = H5Screate(H5S_SCALAR);
            hid_t dsetID = H5Dcreate(m_VDSFile.m_GroupId, var.m_Name.c_str(),
            h5Type, filespaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            //hid_t plistID = H5Pcreate(H5P_DATASET_XFER);
            //H5Pset_dxpl_mpio(plistID, H5FD_MPIO_COLLECTIVE);
            herr_t status = H5Dwrite(dsetID, h5Type, H5S_ALL, H5S_ALL,
            H5P_DEFAULT, values);
            //herr_t status = H5Dwrite(dsetID, h5Type, H5S_ALL, H5S_ALL,
            plistID, values);

            H5Sclose(filespaceID);
            H5Dclose(dsetID);
            */
        }
        return; //
    }

    /* Initialize hyperslab values. */
    size_t all_starts[m_NumSubFiles][nDims];
    size_t all_counts[m_NumSubFiles][nDims];

    //
    std::vector<hsize_t> dimsf, start, one, count;
    GetVarInfo(var, dimsf, nDims, start, count, one);
    //

    m_SubfileComm.Gather(start.data(), nDims, all_starts[0], nDims, 0);
    m_SubfileComm.Gather(count.data(), nDims, all_counts[0], nDims, 0);

    herr_t status;
    if (m_Rank == 0)
    {
        m_VDSFile.CheckWriteGroup();
        /* Set VDS creation property. */
        hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
        // status = H5Pset_fill_value(dcpl, ADIOS2_MPI_SIZE_T, 0);

        space = H5Screate_simple(nDims, dimsf.data(), NULL);
        // summaryFile.Init(fileName.c_str(), MPI_COMM_SELF, true);

        hsize_t currCount[nDims], currStart[nDims];
        // std::string subfileVarName="TimeStep0/"+var.m_Name; // need full
        // path?  NEED TO GET the RIGHT SUBFILE VAR NAME RELATED to TIMESTEP!!
        std::string subfileVarName;
        interop::HDF5Common::StaticGetAdiosStepString(
            subfileVarName, m_VDSFile.m_CurrentAdiosStep);
        subfileVarName += "/" + var.m_Name;

        for (int i = 0; i < m_NumSubFiles; i++)
        {
            for (int j = 0; j < nDims; j++)
            {
                currCount[j] = all_counts[i][j];
                currStart[j] = all_starts[i][j];
                // std::cout<<i<<"th: subfile, "<<j<<"th dirmention:
                // count:"<<currCount[j] <<" start:"<<currStart[j]<<std::endl;
            }
            hid_t src_space = H5Screate_simple(
                nDims, currCount,
                NULL); // with factor=1, we do not flatten the data

            status = H5Sselect_hyperslab(space, H5S_SELECT_SET, currStart, NULL,
                                         one.data(), currCount);

            std::string path, root, subfileName;
            HDFSerialWriter::StaticCreateName(
                path, root, subfileName, m_FileName,
                i); // for each core, get the subfile name

            // std::cout<<" subfileName="<<subfileName<<",
            // var="<<subfileVarName<<std::endl;

            status = H5Pset_virtual(dcpl, space, subfileName.c_str(),
                                    subfileVarName.c_str(), src_space);
            status = H5Sclose(src_space);
        }

        /* Create a virtual dataset. */
        // hid_t dset = H5Dcreate2 (m_VDSFile.m_FileId,  subfileVarName.c_str(),
        // h5Type, space, H5P_DEFAULT, dcpl, H5P_DEFAULT);

        hid_t dset = H5Dcreate2(m_VDSFile.m_GroupId, var.m_Name.c_str(), h5Type,
                                space, H5P_DEFAULT, dcpl, H5P_DEFAULT);

        status = H5Sclose(space);
        status = H5Dclose(dset);
        status = H5Pclose(dcpl);
    }

    // m_VDSFile.Close();
    m_SubfileComm.Barrier();
}

void HDFVDSWriter::Advance(const float timeoutSeconds)
{
    if (m_Rank > 0)
    {
        return;
    }

    m_VDSFile.Advance();
}

void HDFVDSWriter::Close(const int transportIndex)
{
    if (m_Rank > 0)
    {
        return;
    }

    m_VDSFile.Close();
}

//
// class HDFSerialWriter
//
HDFSerialWriter::HDFSerialWriter(helper::Comm const &comm,
                                 const bool debugMode = false)
: m_LocalComm(comm), m_DebugMode(debugMode), m_H5File(debugMode)
{
}

void HDFSerialWriter::Advance(const float timeoutSeconds)
{
    m_H5File.Advance();
}
void HDFSerialWriter::Close(const int transportIndex) { m_H5File.Close(); };

void HDFSerialWriter::StaticCreateName(std::string &pathName,
                                       std::string &rootName,
                                       std::string &fullH5Name,
                                       const std::string &input, int rank)
{

    auto lf_GetBaseName = [](const std::string &name) -> std::string {
        const std::string baseName(adios2::helper::AddExtension(name, ".h5") +
                                   ".dir");
        return baseName;
    };

    auto lf_GetRootTag = [](const std::string &userTag) -> std::string {
        std::string h5RootName = userTag;
        const auto lastPathSeparator(userTag.find_last_of(PathSeparator));
        if (lastPathSeparator != std::string::npos)
        {
            h5RootName = userTag.substr(lastPathSeparator);
        }
        return h5RootName;
    };

    pathName = lf_GetBaseName(input);
    rootName = lf_GetRootTag(input);

    fullH5Name =
        (pathName + "/" + rootName + "_" + std::to_string(rank) + ".h5");
}

void HDFSerialWriter::Init(const std::string &name, int rank)
{
    /*
    auto lf_GetBaseName = [](const std::string &name) -> std::string {
      const std::string baseName(AddExtension(name, ".h5") + ".dir");
      return baseName;
    };

    auto lf_GetRootTag = [] (const std::string &userTag) -> std::string {
      std::string h5RootName = userTag;
      const auto lastPathSeparator(userTag.find_last_of(PathSeparator));
      if (lastPathSeparator != std::string::npos)
        {
          h5RootName = userTag.substr(lastPathSeparator);
        }
      return h5RootName;
    };

    std::string baseName=lf_GetBaseName(name);

    auto rootTag = lf_GetRootTag(name);
    const std::string h5Name(baseName + "/" +
    rootTag+"_"+std::to_string(rank)+".h5");

    */
    std::string baseName, rootTag, h5Name;
    StaticCreateName(baseName, rootTag, h5Name, name, rank);
    // std::cout<<"rank="<<rank<<"  name="<<h5Name<<std::endl;
    adios2::helper::CreateDirectory(baseName);
    m_H5File.Init(h5Name, m_LocalComm, true);

    m_FileName = h5Name;
    m_Rank = rank;
    // m_H5File.Init(h5Name, m_, true);
}

/*
  std::vector<std::string>
    GetBaseNames(const std::vector<std::string> &names) const noexcept
    {
      auto lf_GetBaseName = [](const std::string &name) -> std::string {
        const std::string baseName(AddExtension(name, ".h5") + ".dir");
        return baseName;
      };

      std::vector<std::string> baseNames;
      baseNames.reserve(names.size());

      for (const auto &name : names)
        {
          baseNames.push_back(lf_GetBaseName(name));
        }
      return baseNames;
    }


  std::vector<std::string>
    GetLocalFileNames(const std::vector<std::string> &baseNames,
                      const std::string &userTag) const noexcept
    {
      // e.g. /some/where/xy.h5.dir
      // e.g. xy

      auto lf_GetH5Name = [](const std::string &baseName,
                             const std::string &userTag,
                             const int rank) -> std::string {
#ifdef NEVER
        const std::string h5BaseName = AddExtension(baseName, ".h5");

        std::string h5RootName = h5BaseName;
        const auto lastPathSeparator(h5BaseName.find_last_of(PathSeparator));

        if (lastPathSeparator != std::string::npos)
        {
            h5RootName = h5BaseName.substr(lastPathSeparator);
        }
        const std::string h5Name(h5BaseName + ".dir/" + h5RootName + "." +
                                 std::to_string(rank));
#else
        const std::string h5Name(baseName + "/" +
userTag+"_"+std::to_string(rank)+".h5"); #endif return h5Name;
    };


      auto lf_GetRootTag = [] (const std::string &userTag) -> std::string {
        std::string h5RootName = userTag;
        const auto lastPathSeparator(userTag.find_last_of(PathSeparator));
        if (lastPathSeparator != std::string::npos)
          {
            h5RootName = userTag.substr(lastPathSeparator);
          }
        return h5RootName;
      };

    std::vector<std::string> h5Names;
    h5Names.reserve(baseNames.size());

    auto rootTag = lf_GetRootTag(userTag);
    for (const auto &baseName : baseNames)
    {
      h5Names.push_back(lf_GetH5Name(baseName, rootTag, m_RankMPI));
    }
    return h5Names;

    }


  enum class ResizeResult
  {
    Failure,   //!< FAILURE, caught a std::bad_alloc
    Unchanged, //!< UNCHANGED, no need to resize (sufficient capacity)
    Success,   //!< SUCCESS, resize was successful
    Flush      //!< FLUSH, need to flush to transports for current variable
  };


    template <class T>
      ResizeResult ResizeBuffer(const Variable<T> &variable)
      { std::cout<<"ResizeBuffer() Forcing Flush for now."<<std::endl;
        return HDFSerialWriter::ResizeResult::Flush;};

  capsule::STLVector m_HeapBuffer;
  profiling::IOChrono m_Profiler;

  void InitParameters(const Params &parameters)
  {
    std::cout<<"InitParameters(), empty for now. "<<std::endl;
  };

  //
  // from H51Writer
  //


  std::string GetRankProfilingJSON(
                                   const std::vector<std::string>
&transportsTypes, const std::vector<profiling::IOChrono *> &transportsProfilers)
noexcept
    {
      std::cout<<"GetRankProfilingJSON() returns empty string now "<<std::endl;
      return "";
    }

  std::string
    AggregateProfilingJSON(const std::string &rankProfilingJSON) noexcept
    {
      std::cout<<"AggregateProfilingJSON() to hdf5"<<std::endl;
      return "agg.hd5";
    }

  void WriteProcessGroupIndex(
                              const std::string hostLanguage,
                              const std::vector<std::string> &transportsTypes)
noexcept
  {
      std::cout<<"WriteProcessGroupIndex() to hdf5"<<std::endl;
  }

  void Flush() {
    std::cout<<"Flush() out hdf5"<<std::endl;
  }

  template <class T>
    void WriteVariableMetadata(const Variable<T> &variable) noexcept
    {
      std::cout<<"WriteVariableMetadata() to hdf5"<<std::endl;
    }
  template <class T>
    void WriteVariablePayload(const Variable<T> &variable) noexcept
    {
      std::cout<<"WriteVariablePayload() to hdf5"<<std::endl;
    }

};
*/

} // end namespace engine
} // end namespace core
} // end namespace adios2
