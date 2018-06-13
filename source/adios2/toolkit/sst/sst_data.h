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
    MACRO(MarshalMethod, MarshalMethod, size_t, 0)                             \
    MACRO(RegistrationMethod, RegMethod, size_t, 0)                            \
    MACRO(DataTransport, String, char *, NULL)                                 \
    MACRO(RendezvousReaderCount, Int, int, 1)                                  \
    MACRO(QueueLimit, Int, int, 0)                                             \
    MACRO(DiscardOnQueueFull, Bool, int, 1)                                    \
    MACRO(IsRowMajor, IsRowMajor, int, 0)

typedef enum {
    SstRegisterFile,
    SstRegisterScreen,
    SstRegisterCloud
} SstRegistrationMethod;

typedef enum { SstMarshalFFS, SstMarshalBP } SstMarshalMethod;

struct _SstParams
{
#define declare_struct(Param, Type, Typedecl, Default) Typedecl Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(declare_struct)
#undef declare_struct
};

#endif /* !_SST_DATA_H_ */
