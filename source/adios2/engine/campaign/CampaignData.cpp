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

namespace adios2
{
namespace core
{
namespace engine
{

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
    ch.longhostname = std::string(argv[1]);
    cdp->hosts.push_back(ch);
    return 0;
};

static int sqlcb_directory(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    size_t hostid = helper::StringToSizeT(std::string(argv[0]), "SQL callback convert text to int");
    size_t hostidx = hostid - 1; // SQL rows start from 1, vector idx start from 0
    cdp->directory.push_back(argv[1]);
    cdp->hosts[hostidx].dirIdx.push_back(cdp->directory.size() - 1);
    return 0;
};

static int sqlcb_bpdataset(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignBPDataset cds;
    size_t dsid = helper::StringToSizeT(std::string(argv[0]), "SQL callback convert text to int");
    size_t hostid = helper::StringToSizeT(std::string(argv[1]), "SQL callback convert text to int");
    size_t dirid = helper::StringToSizeT(std::string(argv[2]), "SQL callback convert text to int");
    cds.hostIdx = hostid - 1; // SQL rows start from 1, vector idx start from 0
    cds.dirIdx = dirid - 1;   // SQL rows start from 1, vector idx start from 0
    cds.name = argv[3];

    cds.hasKey = false;
    cds.keyIdx = 0;
    if (cdp->version.version >= 0.2)
    {
        size_t keyid =
            helper::StringToSizeT(std::string(argv[4]), "SQL callback convert text to int");
        cds.hasKey = (keyid); // keyid == 0 means there is no key used
        cds.keyIdx = size_t(keyid - 1);
    }
    if (cdp->version.version >= 0.3)
    {
        cds.uuid = std::string(argv[5]);
    }
    cdp->bpdatasets[dsid] = cds;
    return 0;
};

static int sqlcb_bpfile(void *p, int argc, char **argv, char **azColName)
{
    CampaignData *cdp = reinterpret_cast<CampaignData *>(p);
    CampaignBPFile cf;
    size_t dsid = helper::StringToSizeT(std::string(argv[0]), "SQL callback convert text to int");
    cf.bpDatasetIdx = dsid;
    cf.name = std::string(argv[1]);
    int comp = helper::StringTo<int>(std::string(argv[2]), "SQL callback convert text to int");
    cf.compressed = (bool)comp;
    cf.lengthOriginal =
        helper::StringToSizeT(std::string(argv[3]), "SQL callback convert text to int");
    cf.lengthCompressed =
        helper::StringToSizeT(std::string(argv[4]), "SQL callback convert text to int");
    cf.ctime = helper::StringTo<int64_t>(std::string(argv[5]), "SQL callback convert ctime to int");

    CampaignBPDataset &cds = cdp->bpdatasets[cf.bpDatasetIdx];
    cds.files.push_back(cf);
    return 0;
};

void ReadCampaignData(sqlite3 *db, CampaignData &cd)
{
    int rc;
    char *zErrMsg = 0;
    std::string sqlcmd;

    sqlcmd = "SELECT version FROM info";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_info, &cd, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading info records:" + m);
        sqlite3_free(zErrMsg);
    }

    if (cd.version.version >= 0.2)
    {
        sqlcmd = "SELECT keyid FROM key";
        rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_key, &cd, &zErrMsg);
        if (rc != SQLITE_OK)
        {
            std::cout << "SQL error: " << zErrMsg << std::endl;
            std::string m(zErrMsg);
            helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                                 "SQL error on reading key records:" + m);
            sqlite3_free(zErrMsg);
        }
    }

    sqlcmd = "SELECT hostname, longhostname FROM host";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_host, &cd, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading host records:" + m);
        sqlite3_free(zErrMsg);
    }

    sqlcmd = "SELECT hostid, name FROM directory";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_directory, &cd, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading directory records:" + m);
        sqlite3_free(zErrMsg);
    }

    if (cd.version.version >= 0.3)
    {
        sqlcmd = "SELECT rowid, hostid, dirid, name, keyid, uuid FROM bpdataset";
    }
    else if (cd.version.version >= 0.2)
    {
        sqlcmd = "SELECT rowid, hostid, dirid, name, keyid FROM bpdataset";
    }
    else
    {
        sqlcmd = "SELECT rowid, hostid, dirid, name FROM bpdataset";
    }
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_bpdataset, &cd, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading bpdataset records:" + m);
        sqlite3_free(zErrMsg);
    }

    sqlcmd = "SELECT bpdatasetid, name, compression, lenorig, lencompressed, ctime "
             "FROM bpfile";
    rc = sqlite3_exec(db, sqlcmd.c_str(), sqlcb_bpfile, &cd, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "ReadCampaignData",
                                             "SQL error on reading bpfile records:" + m);
        sqlite3_free(zErrMsg);
    }
}

/* Decompress from in-memory source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files.
   http://www.zlib.net/zlib_how.html */
int inflateToFile(const unsigned char *source, const size_t blobsize, std::ofstream *dest)
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
            dest->write(reinterpret_cast<char *>(out.data()), have);
            if (dest->bad())
            {
                helper::Throw<std::runtime_error>("Core", "Campaign", "Inflate",
                                                  "error writing file ");
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

static bool isFileNewer(const std::string path, int64_t ctime)
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

    int64_t ct = static_cast<int64_t>(s.st_ctime);
    int64_t ctSec = timeToSec(ct);
    int64_t ctimeSec = timeToSec(ctime);

    /*std::cout << "   Stat(" << path << "): size = " << s.st_size
              << " ct = " << ctSec << " ctime = " << ctimeSec << "\n";*/
    return (ctSec > ctimeSec);
}

#ifdef ADIOS2_HAVE_SODIUM
void DecryptData(const unsigned char *encryptedData, size_t lenEncrypted, size_t lenDecrypted,
                 const CampaignBPFile &bpfile, std::string keystr, unsigned char *decryptedData)
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

void SaveToFile(sqlite3 *db, const std::string &path, const CampaignBPFile &bpfile,
                std::string &keyHex)
{
    if (isFileNewer(path, bpfile.ctime))
    {
        return;
    }

    int rc;
    char *zErrMsg = 0;
    std::string sqlcmd;
    std::string id = std::to_string(bpfile.bpDatasetIdx);

    sqlite3_stmt *statement;
    sqlcmd =
        "SELECT data FROM bpfile WHERE bpdatasetid = " + id + " AND name = '" + bpfile.name + "'";
    // std::cout << "SQL statement: " << sqlcmd << "\n";
    rc = sqlite3_prepare_v2(db, sqlcmd.c_str(), static_cast<int>(sqlcmd.size()), &statement, NULL);
    if (rc != SQLITE_OK)
    {
        std::cout << "SQL error: " << zErrMsg << std::endl;
        std::string m(zErrMsg);
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "SaveToFIle",
                                             "SQL error on reading info records:" + m);
        sqlite3_free(zErrMsg);
    }

    int result = 0;
    result = sqlite3_step(statement);
    if (result != SQLITE_ROW)
    {
        helper::Throw<std::invalid_argument>("Engine", "CampaignReader", "SaveToFIle",
                                             "Did not find record for :" + bpfile.name);
    }

    int iBlobsize = sqlite3_column_bytes(statement, 0);
    const void *blob = sqlite3_column_blob(statement, 0);

    /*std::cout << "-- Retrieved from DB data of " << bpfile.name << " size = " << iBlobsize
              << " compressed = " << bpfile.compressed << " encryption key = " << keyHex
              << " compressed size = " << bpfile.lengthCompressed
              << " original size = " << bpfile.lengthOriginal << " blob = " << blob << "\n";*/

    size_t blobsize = static_cast<size_t>(iBlobsize);
    void *q = const_cast<void *>(blob);
    bool free_q = false;

#ifdef ADIOS2_HAVE_SODIUM
    if (!keyHex.empty())
    {
        size_t decryptedSize =
            (bpfile.compressed ? bpfile.lengthCompressed : bpfile.lengthOriginal);
        q = malloc(decryptedSize);
        free_q = true;
        DecryptData(static_cast<const unsigned char *>(blob), blobsize, decryptedSize, bpfile,
                    keyHex, static_cast<unsigned char *>(q));
    }
#endif

    std::ofstream f;
    f.rdbuf()->pubsetbuf(0, 0);
    f.open(path, std::ios::out | std::ios::binary);
    if (bpfile.compressed)
    {
        const unsigned char *ptr = static_cast<const unsigned char *>(q);
        inflateToFile(ptr, blobsize, &f);
    }
    else
    {
        f.write(static_cast<const char *>(q), blobsize);
    }
    f.close();
    if (free_q)
    {
        free(q);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
