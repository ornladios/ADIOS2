#ifndef _SST_DATA_H_
#define _SST_DATA_H_

#ifndef _SYS_TYPES_H_
#include <sys/types.h>
#endif

typedef enum { SST_UINT = 1, SST_INT = 2, SST_FLOAT = 3 } SST_BASE_TYPE;

struct _SstFullMetadata
{
    int WriterCohortSize;
    struct _SstBlock **WriterMetadata;
    void **DP_TimestepInfo;
};

struct _SstMetadata
{
    size_t DataSize;
    int IntVarCount;
    struct _SstIntMeta *IntVars;
    int FloatVarCount;
    struct _SstFloatMeta *FloatVars;
    int VarCount;
    struct _SstVarMeta *Vars;
};

struct _SstData
{
    size_t DataSize;
    char *block;
};

typedef struct _SstBlock
{
    size_t BlockSize;
    char *BlockData;
} * SstBlock;

struct _SstVarMeta
{
    char *VarName;
    int DimensionCount;
    struct _SstDimenMeta *Dimensions;
    int DataOffsetInBlock;
};

struct _SstIntMeta
{
    char *VarName;
    int64_t Value;
};

struct _SstUintMeta
{
    char *VarName;
    u_int64_t Value;
};

struct _SstFloatMeta
{
    char *VarName;
    double Value;
};

struct _SstDimenMeta
{
    int Offset;
    int Size;
    int GlobalSize;
};

#endif /* !_SST_DATA_H_ */
