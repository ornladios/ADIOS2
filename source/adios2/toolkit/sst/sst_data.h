#ifndef _SST_DATA_H_
#define _SST_DATA_H_

#ifndef _SYS_TYPES_H_
#include <sys/types.h>
#endif

typedef enum { SST_UINT = 1, SST_INT = 2, SST_FLOAT = 3 } SST_BASE_TYPE;

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

#endif /* !_SST_DATA_H_ */
