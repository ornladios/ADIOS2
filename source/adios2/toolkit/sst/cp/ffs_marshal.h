#ifndef FFS_MARSHAL_H_
#define FFS_MARSHAL_H_

enum DataType
{
    None,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
    LongDouble,
    FloatComplex,
    DoubleComplex,
    String,
    Compound
};

typedef struct _FFSWriterRec
{
    void *Key;
    int FieldID;
    size_t DataOffset;
    size_t MetaOffset;
    int DimCount;
    int Type;
    char *Name;
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

typedef struct FFSVarRecStruct
{
    void *Variable;
    char *VarName;
    size_t *PerWriterMetaFieldOffset;
    size_t DimCount;
    int Type;
    int ElementSize;
    size_t *GlobalDims;
    size_t *PerWriterBlockStart;
    size_t *PerWriterBlockCount;
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

typedef struct FFSArrayRequestStruct
{
    FFSVarRec VarRec;
    enum FFSRequestTypeEnum RequestType;
    size_t BlockID;
    size_t *Start;
    size_t *Count;
    void *Data;
    struct FFSArrayRequestStruct *Next;
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

struct ControlStruct
{
    int FieldIndex;
    int FieldOffset;
    FFSVarRec VarRec;
    int IsArray;
    int Type;
    int ElementSize;
};

struct ControlInfo
{
    FMFormat Format;
    int ControlCount;
    struct ControlInfo *Next;
    struct ControlStruct Controls[1];
};

struct FFSReaderMarshalBase
{
    int VarCount;
    FFSVarRec *VarList;
    FMContext LocalFMContext;
    FFSArrayRequest PendingVarRequests;

    void **MetadataBaseAddrs;
    size_t *DataSizes;
    FMFieldList *MetadataFieldLists;

    void **DataBaseAddrs;
    FMFieldList *DataFieldLists;

    FFSReaderPerWriterRec *WriterInfo;
    struct ControlInfo *ControlBlocks;
};

extern char *FFS_ZFPCompress(SstStream Stream, const size_t DimCount, int Type,
                             void *Data, const size_t *Count,
                             size_t *ByteCountP);
extern void *FFS_ZFPDecompress(SstStream Stream, const size_t DimCount,
                               int Type, void *bufferIn, const size_t sizeIn,
                               const size_t *Dimensions, attr_list Parameters);
extern int ZFPcompressionPossible(const int Type, const int DimCount);

struct FFSMetadataInfoStruct
{
    size_t BitFieldCount;
    size_t *BitField;
    size_t DataBlockSize;
};

typedef struct _MetaArrayRec
{
    size_t Dims;     // How many dimensions does this array have
    size_t DBCount;  // Dimens * BlockCount
    size_t *Shape;   // Global dimensionality  [Dims]	NULL for local
    size_t *Count;   // Per-block Counts	  [DBCount]
    size_t *Offsets; // Per-block Offsets	  [DBCount]	NULL for local
} MetaArrayRec;

extern int FFSBitfieldTest(struct FFSMetadataInfoStruct *MBase, int Bit);
extern FFSVarRec LookupVarByName(SstStream Stream, const char *Name);
extern FFSVarRec GetVarByNumber(SstStream Stream, int index);
extern void ReverseDimensions(size_t *Dimensions, int count);
extern FFSVarRec CreateVarRec(SstStream Stream, const char *ArrayName);

#endif /* FFS_MARSHAL_H_ */
