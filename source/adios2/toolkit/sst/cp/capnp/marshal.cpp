#include "adios.capnp.h"
#include <capnp/message.h>
#include <capnp/pretty-print.h>
#include <capnp/serialize-packed.h>
#include <iostream>
#include <stdlib.h>

extern "C" {
#include "adios2/common/ADIOSConfig.h"

#include "../../sst.h"
#include "../cp_internal.h"
#include "../ffs_marshal.h"
}
extern "C" void *CapnProtoEncode(SstStream Stream, void *MData,
                                 size_t *DataSizePtr)
{
    int VarCount = 0;
    struct FFSWriterMarshalBase *Info =
        (struct FFSWriterMarshalBase *)Stream->WriterMarshalData;
    for (int i = 0; i < Info->RecCount; i++)
    {
        if (FFSBitfieldTest((struct FFSMetadataInfoStruct *)Stream->M, i))
            VarCount++;
    }
    ::capnp::MallocMessageBuilder message;

    TSGroup::Builder tsgroup = message.initRoot<TSGroup>();
    tsgroup.setDataBlockSize(
        ((struct FFSMetadataInfoStruct *)(Stream->M))->DataBlockSize);
    auto vars = tsgroup.initVariables(VarCount);
    int CPvar = 0;
    for (int i = 0; i < Info->RecCount; i++)
    {
        if (!FFSBitfieldTest((struct FFSMetadataInfoStruct *)Stream->M, i))
            continue;
        FFSWriterRec Rec = &Info->RecList[i];
        TSGroup::VariableInfo::Builder var = vars[CPvar];
        if (Rec->DimCount == 0)
        {
            TSGroup::GlobalVariableInfo::Builder gvar = var.initGlobal();
            gvar.setNumber(i);
            gvar.setName(Rec->Name);
            void *ptr = ((char *)(Stream->M) + Rec->MetaOffset);
            switch (Rec->Type)
            {
            case 0:
                break;
            case Int8: //    Int8
                gvar.setInt8(*(int8_t *)ptr);
                break;
            case Int16: //        Int16
                gvar.setInt16(*(int16_t *)ptr);
                break;
            case Int32: //    Int32
                gvar.setInt32(*(int32_t *)ptr);
                break;
            case Int64: //    Int64
                gvar.setInt64(*(int64_t *)ptr);
                break;
            case UInt8: //    Int8
                gvar.setUint8(*(uint8_t *)ptr);
                break;
            case UInt16: //        Int16
                gvar.setUint16(*(uint16_t *)ptr);
                break;
            case UInt32: //    UInt32
                gvar.setUint32(*(uint32_t *)ptr);
                break;
            case UInt64: //    Uint64
                gvar.setUint64(*(uint64_t *)ptr);
                break;
            case Float: //    Float
                gvar.setFloat(*(float *)ptr);
                break;
            case Double: //        Double
                gvar.setDouble(*(double *)ptr);
                break;
            case LongDouble: //    LongDouble
                             //                gvar.setLongDouble();
                break;
            case FloatComplex: //    FloatComplex
                               //                gvar.setInt64();
                break;
            case DoubleComplex: //    DoubleComplex
                                //                gvar.setInt64();
                break;
            case String: //    String
                         //                gvar.setInt64();
                break;
            }
        }
        else
        {
            MetaArrayRec *MetaEntry =
                (MetaArrayRec *)((char *)(Stream->M) + Rec->MetaOffset);
            TSGroup::ArrayVariableInfo::Builder avar = var.initArray();
            int BlockCount = MetaEntry->DBCount / MetaEntry->Dims;
            avar.setNumber(i);
            avar.setName(Rec->Name);
            avar.setType(Rec->Type);
            if (MetaEntry->Shape)
            {
                auto shape = avar.initShape(MetaEntry->Dims);
                for (int k = 0; k < MetaEntry->Dims; k++)
                {
                    shape.set(k, MetaEntry->Shape[k]);
                }
            }
            auto blks = avar.initBlocks(BlockCount);
            for (int j = 0; j < BlockCount; j++)
            {
                TSGroup::BlocksInfo::Builder bi = blks[j];
                if (MetaEntry->Offsets)
                {
                    auto start = bi.initStart(MetaEntry->Dims);
                    for (int k = 0; k < MetaEntry->Dims; k++)
                    {
                        start.set(k,
                                  MetaEntry->Offsets[j * MetaEntry->Dims + k]);
                    }
                }
                auto count = bi.initCount(MetaEntry->Dims);
                for (int k = 0; k < MetaEntry->Dims; k++)
                {
                    count.set(k, MetaEntry->Count[j * MetaEntry->Dims + k]);
                }
            }
        }

        CPvar++;
    }

    //    auto text = kj::str(capnp::prettyPrint(tsgroup), '\n');
    //    std::string data(text.begin(), text.end());
    //    std::cout << data << std::endl;
    kj::Array<const capnp::word> tmp = capnp::messageToFlatArray(message);
    auto bytes = tmp.asBytes();
    long size = bytes.size();
    unsigned char *ret = (unsigned char *)malloc(size);
    memcpy(ret, bytes.begin(), size);
    *DataSizePtr = size;
    return ret;
}

#define ROUNDUP(n, width) (((n) + (width)-1) & ~unsigned((width)-1))
#define NextOffsetLoc(ptr)                                                     \
    ((size_t *)(((char *)ptr) + ROUNDUP(sizeof(struct FFSMetadataInfoStruct),  \
                                        sizeof(size_t))))
static void *InitMetadataBase(SstStream Stream, int WriterRank,
                              size_t DataBlockSize)
{
    struct FFSReaderMarshalBase *Info =
        (struct FFSReaderMarshalBase *)Stream->ReaderMarshalData;
    int ObjSize = sizeof(struct FFSMetadataInfoStruct) + sizeof(size_t);
    Info->MetadataBaseAddrs[WriterRank] =
        malloc(ROUNDUP(ObjSize, sizeof(size_t)));
    ((struct FFSMetadataInfoStruct *)Info->MetadataBaseAddrs[WriterRank])
        ->DataBlockSize = DataBlockSize;
    *(NextOffsetLoc(Info->MetadataBaseAddrs[WriterRank])) =
        ROUNDUP(ObjSize, sizeof(size_t));
    return Info->MetadataBaseAddrs[WriterRank];
}

static size_t AddDataToMetadataBase(SstStream Stream, int WriterRank,
                                    void *Data, int ElementSize)
{
    struct FFSReaderMarshalBase *Info =
        (struct FFSReaderMarshalBase *)Stream->ReaderMarshalData;
    size_t DataLoc = *(NextOffsetLoc(Info->MetadataBaseAddrs[WriterRank]));
    Info->MetadataBaseAddrs[WriterRank] =
        realloc(Info->MetadataBaseAddrs[WriterRank],
                ROUNDUP(DataLoc + ElementSize, sizeof(size_t)));
    memcpy(((char *)Info->MetadataBaseAddrs[WriterRank]) + DataLoc, Data,
           ElementSize);
    *(NextOffsetLoc(Info->MetadataBaseAddrs[WriterRank])) =
        ROUNDUP(DataLoc + ElementSize, sizeof(size_t));
    return DataLoc;
}

int SizeArray[] = {0, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8, -1, 8, 16, -1, -1};

extern "C" void CapnProtoBuildVarList(SstStream Stream, unsigned char *MData,
                                      size_t Size, int WriterRank)
{
    kj::ArrayPtr<capnp::word> words(reinterpret_cast<capnp::word *>(MData),
                                    Size / sizeof(capnp::word));
    capnp::FlatArrayMessageReader message(words);
    TSGroup::Reader tsgroup = message.getRoot<TSGroup>();
    auto vars = tsgroup.getVariables();
    auto DataBlockSize = tsgroup.getDataBlockSize();
    FFSTypeHandle FFSformat;
    static int DumpMetadata = -1;

    /* incoming metadata is all of our information about what was written
     * and is available to be read.  We'll process the data from each node
     * separately, but in such a way that we don't need to process the
     * MetaData again.  That means keeping around the information that we'll
     * need to respond to Get[Sync,Deferred] actions later.  For this we use
     * the VarList, which is keyed by the address of the Variable object
     * created at the ADIOS2 level.  So, we run through the individual
     * metadata blocks from a rank.  For each field set (one field for
     * atomic values, 4 for arrays), we look to see if we that name already
     * exists in the VarList (never for WriterRank==0), and if not we create
     * it and do the upcall to create the Variable object.  Then for each
     * Variable, we note global geometry, and for each rank we note the
     * FMField descriptor for its entry in the MetaData block and the
     * geometry for that block (Start + Offset arrays).  Also for each rank
     * we track the info that we'll need later (base address of decoded
     * metadata), format lists /buffers that might need freeing, etc.
     */

    struct FFSReaderMarshalBase *Info =
        (struct FFSReaderMarshalBase *)Stream->ReaderMarshalData;
    if (!Info)
    {
        Info = (struct FFSReaderMarshalBase *)malloc(sizeof(*Info));
        memset(Info, 0, sizeof(*Info));
        Stream->ReaderMarshalData = Info;
        Info->WriterInfo = (FFSReaderPerWriterRec *)calloc(
            sizeof(Info->WriterInfo[0]), Stream->WriterCohortSize);
        Info->MetadataBaseAddrs = (void **)calloc(
            sizeof(Info->MetadataBaseAddrs[0]), Stream->WriterCohortSize);
        Info->DataBaseAddrs = (void **)calloc(sizeof(Info->DataBaseAddrs[0]),
                                              Stream->WriterCohortSize);
        Info->DataFieldLists = (FMField **)calloc(
            sizeof(Info->DataFieldLists[0]), Stream->WriterCohortSize);
    }

    if (!MData)
    {
        fprintf(stderr,
                "FAILURE!   MetaData->Metadata[WriterRank]->block == "
                "NULL for WriterRank = %d\n",
                WriterRank);
    }
    if (DumpMetadata == -1)
    {
        DumpMetadata = (getenv("SstDumpMetadata") != NULL);
    }
    if (DumpMetadata && (Stream->Rank == 0))
    {
        printf("\nIncomingMetadatablock from WriterRank %d is %p :\n",
               WriterRank, MData);
        auto text = kj::str(capnp::prettyPrint(tsgroup), '\n');
        std::string data(text.begin(), text.end());
        std::cout << data << std::endl;
    }

    Info->MetadataBaseAddrs[WriterRank] =
        InitMetadataBase(Stream, WriterRank, DataBlockSize);
    for (TSGroup::VariableInfo::Reader var : tsgroup.getVariables())
    {
        // FFSVarRec VarRec = ControlArray[i].VarRec;
        // void *field_data = (char *)BaseData + FieldOffset;
        // if (!FFSBitfieldTest(BaseData, i))
        // {
        //     continue;
        // }
        if (var.which() == TSGroup::VariableInfo::ARRAY)
        {
            TSGroup::ArrayVariableInfo::Reader arr = var.getArray();
            const char *Name = arr.getName().cStr();
            //            FFSVarRec VarRec = LookupVarByName(Stream, Name);
            FFSVarRec VarRec = GetVarByNumber(Stream, arr.getNumber());
            auto shape = arr.getShape();
            if (!VarRec)
            {
                VarRec = CreateVarRec(Stream, Name);
            }
            if (WriterRank == 0)
            {
                if (arr.hasShape())
                {
                    int Dims = shape.size();
                    VarRec->DimCount = Dims;
                    if (!VarRec->GlobalDims)
                        VarRec->GlobalDims = (size_t *)malloc(
                            sizeof(VarRec->GlobalDims[0]) * Dims);
                    for (int i = 0; i < Dims; i++)
                    {
                        VarRec->GlobalDims[i] = shape[i];
                    }
                    if ((Dims > 1) && (Stream->WriterConfigParams->IsRowMajor !=
                                       Stream->ConfigParams->IsRowMajor))
                    {
                        ReverseDimensions(VarRec->GlobalDims, Dims);
                    }
                }
                else
                {
                    VarRec->GlobalDims = NULL;
                }
            }
            auto blocks = arr.getBlocks();
            VarRec->PerWriterBlockCount[WriterRank] = blocks.size();
            for (int i = 0; i < blocks.size(); i++)
            {
                auto block = blocks[i];
                if (block.hasStart())
                {
                    auto start = block.getStart();
                    int Dims = start.size();
                    VarRec->DimCount = Dims;
                    if (!VarRec->PerWriterStart[WriterRank])
                        VarRec->PerWriterStart[WriterRank] = (size_t *)malloc(
                            sizeof(VarRec->GlobalDims[0]) * Dims);
                    else
                        VarRec->PerWriterStart[WriterRank] = (size_t *)realloc(
                            VarRec->PerWriterStart[WriterRank],
                            (i + 1) * sizeof(VarRec->GlobalDims[0]) * Dims);
                    for (int j = 0; j < Dims; j++)
                    {
                        VarRec->PerWriterStart[WriterRank][i * Dims + j] =
                            start[j];
                    }
                    if ((Dims > 1) && (Stream->WriterConfigParams->IsRowMajor !=
                                       Stream->ConfigParams->IsRowMajor))
                    {
                        ReverseDimensions(
                            &VarRec->PerWriterStart[WriterRank][i * Dims],
                            VarRec->DimCount);
                    }
                }
                else
                {
                    VarRec->PerWriterStart = NULL;
                }
                if (block.hasCount())
                {
                    auto count = block.getCount();
                    int Dims = count.size();
                    VarRec->DimCount = Dims;
                    if (!VarRec->PerWriterCounts[WriterRank])
                        VarRec->PerWriterCounts[WriterRank] = (size_t *)malloc(
                            sizeof(VarRec->GlobalDims[0]) * Dims);
                    else
                        VarRec->PerWriterCounts[WriterRank] = (size_t *)realloc(
                            VarRec->PerWriterCounts[WriterRank],
                            (i + 1) * sizeof(VarRec->GlobalDims[0]) * Dims);
                    for (int j = 0; j < Dims; j++)
                    {
                        VarRec->PerWriterCounts[WriterRank][i * Dims + j] =
                            count[j];
                    }
                    if ((Dims > 1) && (Stream->WriterConfigParams->IsRowMajor !=
                                       Stream->ConfigParams->IsRowMajor))
                    {
                        ReverseDimensions(
                            &VarRec->PerWriterCounts[WriterRank][i * Dims],
                            Dims);
                    }
                }
                else
                {
                    VarRec->PerWriterCounts = NULL;
                }
            }
            VarRec->Type = arr.getType();
            VarRec->ElementSize = SizeArray[VarRec->Type];
            if (!VarRec->Variable)
            {
                size_t *Start = NULL;
                if (VarRec->PerWriterStart)
                    Start = VarRec->PerWriterStart[WriterRank];
                VarRec->Variable = Stream->ArraySetupUpcall(
                    Stream->SetupUpcallReader, VarRec->VarName, VarRec->Type,
                    VarRec->DimCount, VarRec->GlobalDims, Start,
                    VarRec->PerWriterCounts[WriterRank]);
            }

            if (WriterRank == 0)
            {
                VarRec->PerWriterBlockStart[WriterRank] = 0;
            }
            if (WriterRank < Stream->WriterCohortSize - 1)
            {
                VarRec->PerWriterBlockStart[WriterRank + 1] =
                    VarRec->PerWriterBlockStart[WriterRank] +
                    VarRec->PerWriterBlockCount[WriterRank];
            }
            if (!getenv("KillBlocksInfo"))
            {
                for (int i = 0; i < blocks.size(); i++)
                {
                    size_t *Offsets = NULL;
                    if (VarRec->PerWriterStart &&
                        VarRec->PerWriterStart[WriterRank])
                        Offsets = &VarRec->PerWriterStart[WriterRank]
                                                         [i * VarRec->DimCount];
                    Stream->ArrayBlocksInfoUpcall(
                        Stream->SetupUpcallReader, VarRec->Variable,
                        VarRec->Type, WriterRank, VarRec->DimCount,
                        VarRec->GlobalDims, Offsets,
                        &VarRec->PerWriterCounts[WriterRank]
                                                [i * VarRec->DimCount]);
                }
            }
        }
        else
        {
            TSGroup::GlobalVariableInfo::Reader global = var.getGlobal();
            const char *Name = global.getName().cStr();
            int Type;
            int ElementSize;
            //            FFSVarRec VarRec = LookupVarByName(Stream, Name);
            FFSVarRec VarRec = GetVarByNumber(Stream, global.getNumber());
            if (!VarRec)
            {
                VarRec = CreateVarRec(Stream, Name);
            }
            char data[16];
            switch (global.which())
            {
            case TSGroup::GlobalVariableInfo::INT8:
                *((int8_t *)&data[0]) = global.getInt8();
                Type = Int8;
                ElementSize = 1;
                break;
            case TSGroup::GlobalVariableInfo::INT16:
                *((int16_t *)&data[0]) = global.getInt16();
                Type = Int16;
                ElementSize = 2;
                break;
            case TSGroup::GlobalVariableInfo::INT32:
                *((int32_t *)&data[0]) = global.getInt32();
                Type = Int32;
                ElementSize = 4;
                break;
            case TSGroup::GlobalVariableInfo::INT64:
                *((int64_t *)&data[0]) = global.getInt64();
                Type = Int64;
                ElementSize = 8;
                break;
            case TSGroup::GlobalVariableInfo::UINT8:
                *((uint8_t *)&data[0]) = global.getUint8();
                Type = UInt8;
                ElementSize = 1;
                break;
            case TSGroup::GlobalVariableInfo::UINT16:
                *((uint16_t *)&data[0]) = global.getUint16();
                Type = UInt16;
                ElementSize = 2;
                break;
            case TSGroup::GlobalVariableInfo::UINT32:
                *((uint32_t *)&data[0]) = global.getUint32();
                Type = UInt32;
                ElementSize = 4;
                break;
            case TSGroup::GlobalVariableInfo::UINT64:
                *((uint64_t *)&data[0]) = global.getUint64();
                Type = UInt64;
                ElementSize = 8;
                break;
            case TSGroup::GlobalVariableInfo::FLOAT:
                *((float *)&data[0]) = global.getFloat();
                Type = Float;
                ElementSize = 4;
                break;
            case TSGroup::GlobalVariableInfo::DOUBLE:
                *((double *)&data[0]) = global.getDouble();
                Type = Double;
                ElementSize = 8;
                break;
#ifdef NOT_DEF
            case TSGroup::GlobalVariableInfo::LONG_DOUBLE:
                *((double *)&data[0]) = global.getLongDouble();
                Type = LongDouble;
                ElementSize = 8;
                break;
            case TSGroup::GlobalVariableInfo::FLOAT_COMPLEX:
                *((double *)&data[0]) = global.getDouble();
                Type = Double;
                ElementSize = 8;
                break;
            case TSGroup::GlobalVariableInfo::DOUBLE_COMPLEX:
                *((double *)&data[0]) = global.getDouble();
                Type = Double;
                ElementSize = 8;
                break;
            case TSGroup::GlobalVariableInfo::String:
                *((double *)&data[0]) = global.getDouble();
                Type = Double;
                ElementSize = 8;
                break;
#endif
            }
            VarRec->Type = Type;
            VarRec->ElementSize = ElementSize;
            if (!VarRec->Variable)
            {
                VarRec->Variable = Stream->VarSetupUpcall(
                    Stream->SetupUpcallReader, VarRec->VarName, VarRec->Type,
                    &data[0]);
            }
            VarRec->PerWriterMetaFieldOffset[WriterRank] =
                AddDataToMetadataBase(Stream, WriterRank, &data[0],
                                      ElementSize);
        }
    }
}
