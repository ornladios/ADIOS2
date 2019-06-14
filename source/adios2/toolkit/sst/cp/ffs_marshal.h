typedef struct _FFSWriterRec
{
    void *Key;
    int FieldID;
    size_t DataOffset;
    size_t MetaOffset;
    int DimCount;
    char *Type;
} * FFSWriterRec;

struct FFSWriterMarshalBase
{
    int RecCount;
    FFSWriterRec RecList;
    FMContext LocalFMContext;
    int MetaFieldCount;
    FMFieldList MetaFields;
    FMFormat MetaFormat;
    int DataFieldCount;
    FMFieldList DataFields;
    FMFormat DataFormat;
    int AttributeFieldCount;
    FMFieldList AttributeFields;
    FMFormat AttributeFormat;
    void *AttributeData;
    int AttributeSize;
    int CompressZFP;
    attr_list ZFPParams;
};

typedef struct FFSVarRec
{
    void *Variable;
    char *VarName;
    FMFieldList *PerWriterMetaFieldDesc;
    FMFieldList *PerWriterDataFieldDesc;
    size_t DimCount;
    char *Type;
    int ElementSize;
    size_t *GlobalDims;
    size_t **PerWriterStart;
    size_t **PerWriterCounts;
    void **PerWriterIncomingData;
    size_t *PerWriterIncomingSize; // important for compression
} * FFSVarRec;

enum FFSRequestTypeEnum
{
    Global = 0,
    Local = 1
};

typedef struct FFSArrayRequest
{
    FFSVarRec VarRec;
    enum FFSRequestTypeEnum RequestType;
    size_t NodeID;
    size_t *Start;
    size_t *Count;
    void *Data;
    struct FFSArrayRequest *Next;
} * FFSArrayRequest;

enum WriterDataStatusEnum
{
    Empty = 0,
    Needed = 1,
    Requested = 2,
    Full = 3
};

typedef struct FFSReaderPerWriterRec
{
    enum WriterDataStatusEnum Status;
    char *RawBuffer;
    DP_CompletionHandle ReadHandle;
} FFSReaderPerWriterRec;

struct FFSReaderMarshalBase
{
    int VarCount;
    FFSVarRec VarList;
    FMContext LocalFMContext;
    FFSArrayRequest PendingVarRequests;

    void **MetadataBaseAddrs;
    FMFieldList *MetadataFieldLists;

    void **DataBaseAddrs;
    FMFieldList *DataFieldLists;

    FFSReaderPerWriterRec *WriterInfo;
};

extern char *FFS_ZFPCompress(SstStream Stream, const size_t DimCount,
                             char *Type, void *Data, const size_t *Count,
                             size_t *ByteCountP);
extern void *FFS_ZFPDecompress(SstStream Stream, const size_t DimCount,
                               char *Type, void *bufferIn, const size_t sizeIn,
                               const size_t *Dimensions, attr_list Parameters);
extern int ZFPcompressionPossible(const char *Type, const int DimCount);
