/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */
#include "Remote.h"
#include "EVPathRemote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"
#ifdef _MSC_VER
#define strdup(x) _strdup(x)
#endif

#define ThrowUp(x)                                                                                 \
    helper::Throw<std::invalid_argument>("Core", "Engine", "ThrowUp",                              \
                                         "Non-overridden function " + std::string(x) +             \
                                             " called in Remote")

namespace adios2
{

void Remote::Open(const std::string hostname, const int32_t port, const std::string filename,
                  const Mode mode, bool RowMajorOrdering)
{
    ThrowUp(("RemoteOpen"));
};

void Remote::OpenSimpleFile(const std::string hostname, const int32_t port,
                            const std::string filename)
{
    ThrowUp("RemoteSimpleOpen");
};

Remote::GetHandle Remote::Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start,
                              void *dest)
{
    ThrowUp("RemoteGet");
    return (Remote::GetHandle)(intptr_t)0;
};

bool Remote::WaitForGet(GetHandle handle)
{
    ThrowUp("RemoteWaitForGet");
    return false;
}

Remote::GetHandle Remote::Read(size_t Start, size_t Size, void *Dest)
{
    ThrowUp("RemoteRead");
    return (Remote::GetHandle)0;
};
Remote::~Remote() {}
Remote::Remote() {}

} // end namespace adios2
