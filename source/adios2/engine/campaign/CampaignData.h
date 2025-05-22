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

#include <sqlite3.h>

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
    std::vector<size_t> dirIdx; // index in CampaignData.directory global list of dirs
};

struct CampaignKey
{
    std::string id;
    std::string keyHex; // sodium key in hex format
};

struct CampaignFile
{
    std::string name;
    size_t datasetIdx; // index of parent CampaignDataset in the map
    bool compressed;
    size_t lengthOriginal;
    size_t lengthCompressed;
    int64_t ctime;
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
        TEXT
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

struct CampaignDataset
{
    std::string uuid;
    std::string name;
    FileFormat format;
    size_t hostIdx;
    size_t dirIdx;
    bool hasKey;
    size_t keyIdx;
    std::vector<CampaignFile> files;
};

struct CampaignVersion
{
    std::string versionStr;
    int major;
    int minor;
    int micro;
    double version;
};

struct CampaignData
{
    CampaignVersion version;
    std::vector<CampaignHost> hosts;
    std::vector<CampaignKey> keys;
    std::vector<std::string> directory;
    std::map<size_t, CampaignDataset> datasets;
};

void ReadCampaignData(sqlite3 *db, CampaignData &cd);

void SaveToFile(sqlite3 *db, const std::string &path, const CampaignFile &file, std::string &keyHex,
                const CampaignData &cd);

// assumed that memory for data is allocated
void ReadToMemory(sqlite3 *db, char *data, const CampaignFile &file, std::string &keyHex,
                  const CampaignData &cd);

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_CAMPAIGDATA_H_ */
