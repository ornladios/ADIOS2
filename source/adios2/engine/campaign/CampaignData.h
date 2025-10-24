/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignData.h
 * Campaign data from database
 *
 *  Created on: May 16, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_CAMPAIGNDATA_H_
#define ADIOS2_ENGINE_CAMPAIGNDATA_H_

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace adios2
{
namespace core
{
namespace engine
{

struct CampaignHost
{
    std::string hostname;
    std::string longhostname;
    std::string defaultProtocol; // empty, or "https" or "s3"
    std::vector<size_t> dirIdx;  // index in CampaignData.directory global list of dirs
};

struct CampaignDirectory
{
    size_t hostIdx;
    std::string path;
    bool archive; // true if this is on an archival storage
    std::string archiveSystemName;
    std::vector<size_t> archiveIDs;
};

struct CampaignKey
{
    std::string id;
    std::string keyHex; // sodium key in hex format
};

struct CampaignFile
{
    std::string name;
    size_t datasetIdx; // index of grandparent CampaignDataset in the map
    size_t replicaIdx; // index of parent CampaignReplica in CampaignDataset
    bool compressed;
    size_t lengthOriginal;
    size_t lengthCompressed;
    int64_t modtime;
    std::string checksum; // SHA1 checksum of file
};

class FileFormat
{
    // See https://stackoverflow.com/questions/21295935/can-a-c-enum-class-have-methods
public:
    enum Value : uint8_t
    {
        Unknown,
        ADIOS,
        HDF5,
        TEXT,
        IMAGE
    };

    FileFormat() = default;
    constexpr FileFormat(Value ff) : value(ff) {}
    FileFormat(const std::string &fmtstr);

    // Prevent usage: if(fileformat)
    explicit operator bool() const = delete;

    // Enables code like
    // 1. ds.format == FileFormat::HDF5
    // 2. switch (ds.format)
    //    {
    //      case FileFormat::HDF5: ...
    constexpr operator Value() const { return value; }

    std::string ToString();

private:
    Value value;
};

struct CampaignReplica
{
    std::string name;
    size_t hostIdx;
    size_t dirIdx;
    size_t archiveIdx; // > 0 means an archived replica
    size_t datasetIdx; // index of parent CampaignDataset in the map
    bool deleted;
    bool hasKey;
    size_t keyIdx;
    size_t size; // replica size on remote location
    std::vector<CampaignFile> files;
    // image replicas have resolution information (ds.format == FileFormat::IMAGE)
    size_t x;
    size_t y;
};

struct CampaignDataset
{
    std::string name;
    std::string uuid;
    size_t tsid;    // time series id, 0 = not part of any time series
    size_t tsorder; // order for a time-series, 0..n-1
    FileFormat format;
    bool deleted;
    std::map<size_t, CampaignReplica> replicas; // indexed by replicaID, 1..n, not contiguous
};

struct CampaignTimeSeries
{
    size_t tsid;
    std::string name;
    // map of datasets of a time-series: <tsorder, datasetIdx>
    // indexed by tsorder, 0..n-1, not contiguous
    std::map<size_t, size_t> datasets;
};

struct CampaignVersion
{
    std::string versionStr;
    int major;
    int minor;
    int micro;
    double version;
};

class CampaignData
{
public:
    /* hosts, keys and directories are indexed from 0..n-1 in this class,
       so the database values are converted from 1..n to 0..n-1,
       for easy addressing in vectors.
       Hosts, keys and dirs are never deleted from the database.

       Datasets and their replicas are stored in a map, indexed by their
       rowid values (1..n). Deleted items still exists in database but will
       not be added to the map here.

       File's datasetIdx and replicaIdx are 1..n as in the database.

       TimeSeries is a series of Datasets with a given tsid, ordered by tsorder (0..n-1).
    */
    CampaignVersion version;
    std::vector<CampaignHost> hosts;
    std::vector<CampaignKey> keys;
    std::vector<CampaignDirectory> directory;
    std::map<size_t, CampaignDataset> datasets;      // indexed by datasetID, 1..n, not contiguous
    std::map<size_t, CampaignTimeSeries> timeseries; // indexed by tsid, 1..n, not contiguous
    std::map<size_t, std::string> tarnames;          // indexed by rowid of archive table

    CampaignData() = default;
    ~CampaignData() = default;

    void Open(const std::string path);
    void ReadDatabase();
    void Close();

    void SaveToFile(const std::string &path, const CampaignFile &file, std::string &keyHex);

    // assumed that memory for data is allocated
    void ReadToMemory(char *data, const CampaignFile &file, std::string &keyHex);

    // return 0 if there is no replica found for 'hostname', otherwise the replica index
    size_t FindReplicaOnHost(const size_t datasetIdx, std::string hostname);

private:
    void DumpToFileOrMemory(const CampaignFile &file, std::string &keyHex, const std::string &path,
                            char *data);
    void *m_DB = nullptr;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGDATA_H_ */
