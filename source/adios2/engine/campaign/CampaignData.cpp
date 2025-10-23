/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CampaignData.cpp
 * Campaign data struct
 *
 *  Created on: May 16, 2023
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#include "CampaignData.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#ifdef ADIOS2_HAVE_SODIUM
#include <sodium.h>
#include <string.h>
#endif

#include <sqlite3.h>

namespace adios2
{
namespace core
{
namespace engine
{

static inline sqlite3 *get_impl(void *v) { return reinterpret_cast<sqlite3 *>(v); }

const std::string hint_text_to_int = "SQL callback convert text to int";

/*
 * Data from processes to be recorded
 */

static int sqlcb_info(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    cdp->version.versionStr = std::string(argv[0]);

    char rest[32];
    std::sscanf(cdp->version.versionStr.c_str(), "%d.%d%s", &cdp->version.major,
                &cdp->version.minor, rest);
    if (rest[0])
    {
        sscanf(rest, ".%d%*s", &cdp->version.micro);
    }
    cdp->version.version = double(cdp->version.major);
    if (cdp->version.minor < 10)
        cdp->version.version += double(cdp->version.minor) / 10.0;
    else if (cdp->version.minor < 100)
        cdp->version.version += double(cdp->version.minor) / 100.0;

    return 0;
};

static int sqlcb_key(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignKey ck;
    ck.id = std::string(argv[0]);
    cdp->keys.push_back(ck);
    return 0;
};

static int sqlcb_host(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignHost ch;
    /*
    std::cout << "SQL: host record: ";
    for (int i = 0; i < argc; i++)
    {
        std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL")
                  << std::endl;
    }
    std::cout << std::endl;
    */
    ch.hostname = std::string(argv[0]);
    if (argv[1])
        ch.longhostname = std::string(argv[1]);
    if (argv[2])
        ch.defaultProtocol = std::string(argv[2]);
    cdp->hosts.push_back(ch);
    return 0;
};

static int sqlcb_directory(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignDirectory cd;
    size_t hostid = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    cd.hostIdx = hostid - 1; // SQL rows start from 1, vector idx start from 0
    cd.path = argv[1];
    cd.archive = false;
    cdp->directory.push_back(cd);
    cdp->hosts[cd.hostIdx].dirIdx.push_back(cdp->directory.size() - 1);
    return 0;
};

static int sqlcb_archivedirectory(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    size_t archiveid = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    size_t dirid = helper::StringToSizeT(std::string(argv[1]), hint_text_to_int);
    size_t dirIdx = dirid - 1; // SQL rows start from 1, vector idx start from 0
    cdp->directory[dirIdx].archive = true;
    std::string tarname(argv[2]);
    if (!tarname.empty())
    {
        cdp->tarnames[archiveid] = std::string(argv[2]);
        cdp->directory[dirIdx].archiveIDs.push_back(archiveid);
    }
    cdp->directory[dirIdx].archiveSystemName = std::string(argv[3]);
    return 0;
};

static int sqlcb_timeseries(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignTimeSeries cdts;
    cdts.tsid = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    cdts.name = argv[1];
    cdp->timeseries[cdts.tsid] = cdts;
    return 0;
};

static int sqlcb_dataset(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignDataset cds;
    size_t dsid = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    cds.name = argv[1];
    cds.uuid = std::string(argv[2]);
    cds.deleted = (std::strcmp("0", argv[3]) != 0);
    cds.format = FileFormat(std::string(argv[4]));
    cds.tsid = helper::StringToSizeT(std::string(argv[5]), hint_text_to_int);
    cds.tsorder = helper::StringToSizeT(std::string(argv[6]), hint_text_to_int);
    if (cds.tsid)
    {
        cdp->timeseries[cds.tsid].datasets[cds.tsorder] = dsid;
    }
    cdp->datasets[dsid] = cds;
    return 0;
};

static int sqlcb_replica(void *p, int argc, char **argv, char **azColName)
{
    CampaignDataset *cds = reinterpret_cast<CampaignDataset *>(p);
    CampaignReplica cdr;
    size_t repid = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    size_t dsid = helper::StringToSizeT(std::string(argv[1]), hint_text_to_int);
    size_t hostid = helper::StringToSizeT(std::string(argv[2]), hint_text_to_int);
    size_t dirid = helper::StringToSizeT(std::string(argv[3]), hint_text_to_int);
    size_t archiveid = helper::StringToSizeT(std::string(argv[4]), hint_text_to_int);
    cdr.hostIdx = hostid - 1; // SQL rows start from 1, vector idx start from 0
    cdr.dirIdx = dirid - 1;   // SQL rows start from 1, vector idx start from 0
    cdr.datasetIdx = dsid;
    cdr.archiveIdx = archiveid;
    cdr.name = argv[5];
    cdr.deleted = (std::strcmp("0", argv[6]) != 0);
    cdr.hasKey = false;
    cdr.keyIdx = 0;
    size_t keyid = helper::StringToSizeT(std::string(argv[7]), hint_text_to_int);
    cdr.hasKey = (keyid); // keyid == 0 means there is no key used
    cdr.keyIdx = size_t(keyid - 1);
    cdr.size = helper::StringToSizeT(std::string(argv[8]), hint_text_to_int);
    cds->replicas[repid] = cdr;
    return 0;
};

static int sqlcb_file(void *p, int argc, char **argv, char **azColName)
{
    CampaignDataset *cds = reinterpret_cast<CampaignDataset *>(p);
    CampaignFile cf;
    cf.replicaIdx = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    cf.name = std::string(argv[1]);
    int comp = helper::StringTo<int>(std::string(argv[2]), hint_text_to_int);
    cf.compressed = (bool)comp;
    cf.lengthOriginal = helper::StringToSizeT(std::string(argv[3]), hint_text_to_int);
    cf.lengthCompressed = helper::StringToSizeT(std::string(argv[4]), hint_text_to_int);
    cf.modtime =
        helper::StringTo<int64_t>(std::string(argv[5]), "SQL callback convert modtime to int");
    cf.checksum = argv[6];

    CampaignReplica &cdr = cds->replicas[cf.replicaIdx];
    cf.datasetIdx = cdr.datasetIdx;
    cdr.files.push_back(cf);
    return 0;
};

static int sqlcb_resolution(void *p, int argc, char **argv, char **azColName)
{
    CampaignDataset *cds = reinterpret_cast<CampaignDataset *>(p);
    size_t replicaIdx = helper::StringToSizeT(std::string(argv[0]), hint_text_to_int);
    CampaignReplica &cdr = cds->replicas[replicaIdx];
    cdr.x = helper::StringToSizeT(std::string(argv[1]), hint_text_to_int);
    cdr.y = helper::StringToSizeT(std::string(argv[2]), hint_text_to_int);
    return 0;
};

void CampaignData::Open(const std::string path)
{
    sqlite3 *db;
    int rc = sqlite3_open(path.c_str(), &db);
    if (rc)
    {
        std::string dbmsg(sqlite3_errmsg(db));
        sqlite3_close(db);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "Open",
                                             "Cannot open database" + path + ": " + dbmsg);
    }
    m_DB = reinterpret_cast<void *>(db);
}

void CampaignData::ReadDatabase()
{
    sqlite3 *db = get_impl(m_DB);
    int rc;
    char *zErrMsg = 0;
    std::string sqlcmd;

    sqlcmd = "SELECT version FROM info";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_info, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'info' records:" + m);
        sqlite3_free(zErrMsg);
    }

    if (version.version < 0.6)
    {
        helper::Throw<std::invalid_argument>(
            "Engine", "CampaignReader", "ReadCampaignData",
            "Minimum ACA version supported is 0.6, this file has version:" + version.versionStr);
    }

    sqlcmd = "SELECT keyid FROM key";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_key, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'key' records:" + m);
        sqlite3_free(zErrMsg);
    }

    sqlcmd = "SELECT hostname, longhostname, default_protocol FROM host";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_host, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'host' records:" + m);
        sqlite3_free(zErrMsg);
    }

    sqlcmd = "SELECT hostid, name FROM directory";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_directory, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'directory' records:" + m);
        sqlite3_free(zErrMsg);
    }

    sqlcmd = "SELECT rowid, dirid, tarname, system FROM archive";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_archivedirectory, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'archive' directory records:" +
                                                 m);
        sqlite3_free(zErrMsg);
    }

    /* Get time-series */
    sqlcmd = "SELECT tsid, name FROM timeseries";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_timeseries, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'timeseries' records:" + m);
        sqlite3_free(zErrMsg);
    }

    /* Get datasets filtering out the deleted ones */
    sqlcmd = "SELECT rowid, name, uuid, deltime, fileformat, tsid, tsorder FROM dataset where "
             "deltime = 0";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_dataset, this, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading 'dataset' records:" + m);
        sqlite3_free(zErrMsg);
    }

    for (auto &it : datasets)
    {
        size_t dsIdx = it.first;
        CampaignDataset &ds = it.second;

        /* Get replicas of each dataset filtering out the deleted ones */
        sqlcmd =
            "SELECT rowid, datasetid, hostid, dirid, archiveid, name, deltime, keyid, size FROM "
            "replica "
            "where deltime = 0 and datasetid = " +
            std::to_string(dsIdx);
        rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_replica, &ds, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::cout << "SQL error: " << zErrMsg << std::endl;
            std::string m(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                                 "SQL error on reading 'replica' records:" + m);
            sqlite3_free(zErrMsg);
        }

        for (auto &it : datasets[dsIdx].replicas)
        {
            size_t repIdx = it.first;
            /* Get files of each replica filtering out the deleted ones */
            sqlcmd =
                "SELECT replicaid, name, compression, lenorig, lencompressed, modtime, checksum "
                "FROM file where replicaid = " +
                std::to_string(repIdx);

            rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_file, &ds, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                std::cout << "SQL error: " << zErrMsg << std::endl;
                std::string m(zErrMsg);
                helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                                     "SQL error on reading 'file' records:" + m);
                sqlite3_free(zErrMsg);
            }

            if (ds.format == FileFormat::IMAGE)
            {
                sqlcmd = "SELECT replicaid, x, y FROM resolution where replicaid = " +
                         std::to_string(repIdx);
                rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_resolution, &ds, &zErrMsg);
                if (rc != SQLITE_OK)
                {
                    std::cout << "SQL error: " << zErrMsg << std::endl;
                    std::string m(zErrMsg);
                    helper::Throw<std::invalid_argument>(
                        "Engine", "CampaignReader", "ReadCampaignData",
                        "SQL error on reading 'resolution' records:" + m);
                    sqlite3_free(zErrMsg);
                }
            }
        }
    }
}

void CampaignData::Close()
{
    sqlite3 *db = get_impl(m_DB);
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

/* Decompress from in-memory source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files.
   http://www.zlib.net/zlib_how.html */
int inflateToFileOrMemory(const unsigned char *source, const size_t blobsize,
                          std::ofstream *destFile, char *destData = nullptr)
{
    constexpr size_t CHUNK = 16777216;
    int ret;
    unsigned have;
    z_stream strm;

    std::vector<unsigned char> out(CHUNK);

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    unsigned char *p = const_cast<unsigned char *>(source);
    uInt pos = 0;
    do
    {
        uInt CHUNK_SIZE = static_cast<uInt>(blobsize > CHUNK ? CHUNK : blobsize);
        strm.avail_in = CHUNK_SIZE;

        strm.next_in = p + pos;

        /* run inflate() on input until output buffer not full */
        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out.data();
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret)
            {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR; /* and fall through */
            case Z_STREAM_ERROR:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (destData)
            {
                memcpy(destData, out.data(), have);
                destData += have;
            }
            else
            {
                destFile->write(reinterpret_cast<char *>(out.data()), have);
                if (destFile->bad())
                {
                    helper::Throw<std::runtime_error>("Core", "Campaign", "Inflate",
                                                      "error writing file ");
                }
            }

        } while (strm.avail_out == 0);
        pos += CHUNK_SIZE;
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

static int64_t timeToSec(int64_t ct)
{
    int64_t t;
    if (ct > 99999999999999999)
    {
        /* nanosec to sec */
        t = ct / 1000000000;
    }
    else if (ct > 99999999999999)
    {
        /* microsec to sec */
        t = ct / 1000000;
    }
    else if (ct > 99999999999)
    {
        /* millisec to sec */
        t = ct / 1000;
    }
    else
    {
        t = ct;
    }
    return t;
}

static bool isFileNewer(const std::string path, int64_t mtime)
{
    int result;
#ifdef _WIN32
    struct _stat s;
    result = _stat(path.c_str(), &s);
#else
    struct stat s;
    result = stat(path.c_str(), &s);
#endif
    if (result != 0)
    {
        return false;
    }

    int64_t mt = static_cast<int64_t>(s.st_mtime);
    int64_t mtSec = timeToSec(mt);
    int64_t mtimeSec = timeToSec(mtime);

    /*std::cout << "   Stat(" << path << "): size = " << s.st_size
              << " mt = " << mtSec << " mtime = " << mtimeSec << "\n";*/
    return (mtSec > mtimeSec);
}

#ifdef ADIOS2_HAVE_SODIUM
void DecryptData(const unsigned char *encryptedData, size_t lenEncrypted, size_t lenDecrypted,
                 const CampaignFile &bpfile, std::string keystr, unsigned char *decryptedData)
{
    if (sodium_init() < 0)
    {
        helper::Throw<std::runtime_error>("Engine", "CampaignReader", "InitTransports",
                                          "libsodium could not be initialized");
    }
    unsigned char key[crypto_secretbox_KEYBYTES];

    size_t bin_len;
    sodium_hex2bin(key, crypto_secretbox_KEYBYTES, keystr.c_str(), keystr.size(), nullptr, &bin_len,
                   nullptr);

    if (bin_len != crypto_secretbox_KEYBYTES)
    {
        helper::Throw<std::runtime_error>("Engine", "CampaignReader", "InitTransports",
                                          "Decoding hex key string " + keystr + " failed");
    }

    // grab the nonce ptr
    size_t offset = 0;
    const unsigned char *nonce = reinterpret_cast<const unsigned char *>(encryptedData + offset);
    offset += crypto_secretbox_NONCEBYTES;

    // grab the cipher text ptr
    size_t cipherTextSize = lenDecrypted + crypto_secretbox_MACBYTES;
    const unsigned char *cipherText =
        reinterpret_cast<const unsigned char *>(encryptedData + offset);
    offset += cipherTextSize;

    if (offset != lenEncrypted)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "CampaignReader", "InitTransports",
            "Encrypted data size for file " + bpfile.name +
                " in the campaign database does not match expectations");
    }

    // decrypt directly into dataOut buffer
    if (crypto_secretbox_open_easy(decryptedData, cipherText, cipherTextSize, nonce, key) != 0)
    {
        helper::Throw<std::runtime_error>("Engine", "CampaignReader", "InitTransports",
                                          "Encrypted data of file " + bpfile.name +
                                              " in the campaign database could not be decrypted");
    }
}
#endif

void CampaignData::DumpToFileOrMemory(const CampaignFile &file, std::string &keyHex,
                                      const std::string &path, char *data)
{
    if (!path.empty() && isFileNewer(path, file.modtime))
    {
        return;
    }

    sqlite3 *db = get_impl(m_DB);
    int rc;
    std::string sqlcmd;
    std::string datasetIdx = std::to_string(file.datasetIdx);
    std::string replicaIdx = std::to_string(file.replicaIdx);

    sqlite3_stmt *statement;
    sqlcmd =
        "SELECT data FROM file WHERE replicaid = " + replicaIdx + " AND name = '" + file.name + "'";
    // std::cout << "SQL statement: " << sqlcmd << "\n";
    rc = sqlite3_prepare_v2(db, sqlcmd.c_str(), static_cast<int>(sqlcmd.size()), &statement, NULL);
    if (rc != SQLITE_OK)
    {
        const char *zErrMsg = sqlite3_errmsg(db);
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "SaveToFIle",
                                             "SQL error on reading info records:" + m);
    }

    int result = 0;
    result = sqlite3_step(statement);
    if (result != SQLITE_ROW)
    {
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "SaveToFIle",
                                             "Did not find record for :" + file.name);
    }

    int iBlobsize = sqlite3_column_bytes(statement, 0);
    const void *blob = sqlite3_column_blob(statement, 0);

    /*
    std::cout << "-- Retrieved from DB data of " << file.name << " size = " << iBlobsize
              << " compressed = " << file.compressed << " encryption key = " << keyHex
              << " compressed size = " << file.lengthCompressed
              << " original size = " << file.lengthOriginal << " blob = " << blob << "\n";
    */

    size_t blobsize = static_cast<size_t>(iBlobsize);
    void *q = const_cast<void *>(blob);
    bool free_q = false;

#ifdef ADIOS2_HAVE_SODIUM
    if (!keyHex.empty())
    {
        size_t decryptedSize = (file.compressed ? file.lengthCompressed : file.lengthOriginal);
        q = malloc(decryptedSize);
        free_q = true;
        DecryptData(static_cast<const unsigned char *>(blob), blobsize, decryptedSize, file, keyHex,
                    static_cast<unsigned char *>(q));
    }
#endif

    if (path.empty())
    {
        // read into memory
        if (file.compressed)
        {
            const unsigned char *ptr = static_cast<const unsigned char *>(q);
            inflateToFileOrMemory(ptr, blobsize, nullptr, data);
        }
        else
        {
            memcpy(data, q, blobsize);
        }
    }
    else
    {
        // Save to File
        std::ofstream f;
        f.rdbuf()->pubsetbuf(0, 0);
        f.open(path, std::ios::out | std::ios::binary);
        if (file.compressed)
        {
            const unsigned char *ptr = static_cast<const unsigned char *>(q);
            inflateToFileOrMemory(ptr, blobsize, &f, nullptr);
        }
        else
        {
            f.write(static_cast<const char *>(q), blobsize);
        }
        f.close();
    }

    if (free_q)
    {
        free(q);
    }
}

void CampaignData::SaveToFile(const std::string &path, const CampaignFile &file,
                              std::string &keyHex)
{
    DumpToFileOrMemory(file, keyHex, path, nullptr);
}

void CampaignData::ReadToMemory(char *data, const CampaignFile &file, std::string &keyHex)
{
    DumpToFileOrMemory(file, keyHex, "", data);
}

size_t CampaignData::FindReplicaOnHost(const size_t datasetIdx, std::string hostname)
{
    for (auto &it : datasets[datasetIdx].replicas)
    {
        auto repIdx = it.first;
        auto &rep = it.second;
        if (hosts[rep.hostIdx].hostname == hostname)
        {
            return repIdx;
        }
    }
    return 0;
}

std::string FileFormat::ToString()
{
    switch (value)
    {
    case FileFormat::ADIOS:
        return "ADIOS";
    case FileFormat::HDF5:
        return "HDF5";
    case FileFormat::TEXT:
        return "TEXT";
    case FileFormat::IMAGE:
        return "IMAGE";
    default:
        return "Unknown";
    }
};

FileFormat::FileFormat(const std::string &fmtstr)
{
    if (fmtstr == "ADIOS")
    {
        value = FileFormat::ADIOS;
    }
    else if (fmtstr == "HDF5")
    {
        value = FileFormat::HDF5;
    }
    else if (fmtstr == "TEXT")
    {
        value = FileFormat::TEXT;
    }
    else if (fmtstr == "IMAGE")
    {
        value = FileFormat::IMAGE;
    }
    else
    {
        value = FileFormat::Unknown;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
