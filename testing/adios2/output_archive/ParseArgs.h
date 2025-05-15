#ifndef _WIN32
#include "strings.h"
#else
#define strcasecmp _stricmp
#endif
std::string fname = "ADIOS2Common";
std::string engine = "sst";
adios2::Params engineParams = {}; // parsed from command line

#ifndef TESTING_ADIOS2_ENGINE_COMMON_TESTDATA_H_
// Usually we get this from TestData.h, but not needed everywhere.
std::size_t Nx = 10;
#endif
bool SharedIO = false;
bool SharedVar = false;
int NSteps = 10;
int DurationSeconds = 60 * 60 * 24 * 365; // one year default
int DelayMS = 1000;                       // one step per sec default
int Latest = 0;
int Discard = 0;
int IncreasingDelay = 0;
int NonBlockingBeginStep = 0;
int CompressSz = 0;
int CompressZfp = 0;
int TimeGapExpected = 0;
int IgnoreTimeGap = 1;
int ExpectWriterFailure = 0;
int ExpectOpenTimeout = 0;
int ZeroDataVar = 0;
int ZeroDataRank = 0;
int DelayWhileHoldingStep = 0;
int LongFirstDelay = 0;
int FirstTimestepMustBeZero = 0;
int LockGeometry = 0;
bool VaryingDataSize = false;
bool TestVarDestruction = false;
bool AdvancingAttrs = false;
int NoData = 0;
int NoDataNode = -1;
int Flush = 0;
int EarlyExit = 0;
int LocalCount = 1;
int DataSize = 5 * 1024 * 1024 / 8; /* DefaultMinDeferredSize is 4*1024*1024
                                       This should be more than that. */
bool ModifiableAttributes = false;
bool RoundRobin = false;
bool OnDemand = false;
bool DontClose = false;

std::string shutdown_name = "DieTest";
adios2::Mode GlobalWriteMode = adios2::Mode::Deferred;
adios2::Mode GlobalReadMode = adios2::Mode::Deferred;

static std::string Trim(std::string &str)
{
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

/*
 * Engine parameters spec is a poor-man's JSON.  name:value pairs are separated
 * by equal.  White space is trimmed off front and back.  No quotes or anything
 * fancy allowed.
 */
static adios2::Params ParseEngineParams(std::string Input)
{
    std::istringstream ss(Input);
    std::string Param;
    adios2::Params Ret = {};

    while (std::getline(ss, Param, ','))
    {
        std::istringstream ss2(Param);
        std::string ParamName;
        std::string ParamValue;
        std::getline(ss2, ParamName, '=');
        if (!std::getline(ss2, ParamValue, '='))
        {
            throw std::invalid_argument("Engine parameter \"" + Param + "\" missing value");
        }
        Ret[Trim(ParamName)] = Trim(ParamValue);
    }
    return Ret;
}

