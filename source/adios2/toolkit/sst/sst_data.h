#ifndef _SST_DATA_H_
#define _SST_DATA_H_

#ifndef _SYS_TYPES_H_
#include <sys/types.h>
#endif

struct _SstFullMetadata
{
    int WriterCohortSize;
    struct _SstData **WriterMetadata;
    void **DP_TimestepInfo;
    void *FreeBlock;
};

struct _SstData
{
    size_t DataSize;
    char *block;
};

struct _SstBlock
{
    size_t BlockSize;
    char *BlockData;
};

#define SST_FOREACH_PARAMETER_TYPE_4ARGS(MACRO)                                \
    MACRO(MarshalMethod, MarshalMethod, size_t, SstMarshalBP)                  \
    MACRO(RegistrationMethod, RegMethod, size_t, 0)                            \
    MACRO(DataTransport, String, char *, NULL)                                 \
    MACRO(OpenTimeoutSecs, Int, int, 60)                                       \
    MACRO(RendezvousReaderCount, Int, int, 1)                                  \
    MACRO(QueueLimit, Int, int, 0)                                             \
    MACRO(ReserveQueueLimit, Int, int, 0)                                      \
    MACRO(QueueFullPolicy, QueueFullPolicy, size_t, 0)                         \
    MACRO(IsRowMajor, IsRowMajor, int, 0)                                      \
    MACRO(FirstTimestepPrecious, Bool, int, 0)                                 \
    MACRO(ControlTransport, String, char *, NULL)                              \
    MACRO(NetworkInterface, String, char *, NULL)                              \
    MACRO(ControlInterface, String, char *, NULL)                              \
    MACRO(DataInterface, String, char *, NULL)                                 \
    MACRO(CPCommPattern, CPCommPattern, size_t, SstCPCommMin)                  \
    MACRO(CompressionMethod, CompressionMethod, size_t, 0)                     \
    MACRO(AlwaysProvideLatestTimestep, Bool, int, 0)                           \
    MACRO(SpeculativePreloadMode, SpecPreloadMode, int, SpecPreloadAuto)       \
    MACRO(SpecAutoNodeThreshold, Int, int, 1)                                  \
    MACRO(ControlModule, String, char *, NULL)

typedef enum
{
    SstRegisterFile,
    SstRegisterScreen,
    SstRegisterCloud
} SstRegistrationMethod;

typedef enum
{
    SpecPreloadOff,
    SpecPreloadOn,
    SpecPreloadAuto
} SpeculativePreloadMode;

struct _SstParams
{
#define declare_struct(Param, Type, Typedecl, Default) Typedecl Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(declare_struct)
#undef declare_struct
};

#endif /* !_SST_DATA_H_ */
