/*
 *  The SST external interfaces.
 *
 *  This is more a rough sketch than a final version.  The details will
 *  change when the integration with ADIOS2 layers happen.  In the meantime,
 *  this interface (hopefully) captures enough of the functionality for
 *  control plane and data plane implementations to proceed while the
 *  integration details are hashed out.
 */
#ifndef SST_H_
#define SST_H_

#include "adios2/helper/mpiwrap.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/*!
 * SstStream is the basic type of a stream connecting an ADIOS2 reader
 * and an ADIOS2 writer.  Externally the same data type is used for both.
 */
typedef struct _SstStream *SstStream;

/*
 *  metadata and typedefs are tentative and may come from ADIOS2 constructors.
 */
typedef struct _SstFullMetadata *SstFullMetadata;
typedef struct _SstData *SstData;

typedef enum
{
    SstSuccess,
    SstEndOfStream,
    SstFatalError,
    SstTimeout
} SstStatusValue;

/* The SST version of enum class StepMode in ADIOSTypes.h */
typedef enum
{
    SstAppend, // writer modes ignored in SST
    SstUpdate, // writer modes ignored in SST
    SstNextAvailable,
    SstLatestAvailable // reader advance mode
} SstStepMode;

/*
 * Struct that represents statistics tracked by SST
 */
typedef struct _SstStats
{
    double OpenTimeSecs;
    double CloseTimeSecs;
    double ValidTimeSecs;
    size_t BytesTransferred;
} * SstStats;

typedef struct _SstParams *SstParams;

typedef enum
{
    SstMarshalFFS,
    SstMarshalBP
} SstMarshalMethod;

typedef enum
{
    SstCPCommMin,
    SstCPCommPeer
} SstCPCommPattern;

typedef enum
{
    SstQueueFullBlock = 0,
    SstQueueFullDiscard = 1
} SstQueueFullPolicy;

typedef enum
{
    SstCompressNone = 0,
    SstCompressZFP = 1
} SstCompressionMethod;

/*
 *  Writer-side operations
 */
extern SstStream SstWriterOpen(const char *filename, SstParams Params,
                               MPI_Comm comm);

extern void SstStreamDestroy(SstStream Stream);

typedef void (*DataFreeFunc)(void *Data);
extern void SstProvideTimestep(SstStream s, SstData LocalMetadata,
                               SstData LocalData, long Timestep,
                               DataFreeFunc FreeData, void *FreeClientData,
                               SstData AttributeData,
                               DataFreeFunc FreeAttribute,
                               void *FreeAttributeClientData);
extern void SstWriterClose(SstStream stream);
/*  SstWriterDefinitionLock is called once only, on transition from unlock to
 * locked definitions */
extern void SstWriterDefinitionLock(SstStream stream, long EffectiveTimestep);

/*
 *  Reader-side operations
 */
extern SstStream SstReaderOpen(const char *filename, SstParams Params,
                               MPI_Comm comm);
extern void SstReaderGetParams(SstStream stream,
                               SstMarshalMethod *WriterMarshalMethod);
extern SstFullMetadata SstGetCurMetadata(SstStream stream);
extern void *SstReadRemoteMemory(SstStream s, int rank, long timestep,
                                 size_t offset, size_t length, void *buffer,
                                 void *DP_TimestepInfo);
extern SstStatusValue SstWaitForCompletion(SstStream stream, void *completion);
extern void SstReleaseStep(SstStream stream);
extern SstStatusValue SstAdvanceStep(SstStream stream, const float timeout_sec);
extern void SstReaderClose(SstStream stream);
extern long SstCurrentStep(SstStream s);
/*  SstReaderDefinitionLock is called once only, on transition from unlock to
 * locked definitions */
extern void SstReaderDefinitionLock(SstStream stream, long EffectiveTimestep);

/*
 *  Calls that support FFS-based marshaling, source code in cp/ffs_marshal.c
 */
typedef void *(*VarSetupUpcallFunc)(void *Reader, const char *Name,
                                    const char *Type, void *Data);
typedef void (*AttrSetupUpcallFunc)(void *Reader, const char *Name,
                                    const char *Type, void *Data);
typedef void *(*ArraySetupUpcallFunc)(void *Reader, const char *Name,
                                      const char *Type, int DimsCount,
                                      size_t *Shape, size_t *Start,
                                      size_t *Count);
extern void SstReaderInitFFSCallback(SstStream stream, void *Reader,
                                     VarSetupUpcallFunc VarCallback,
                                     ArraySetupUpcallFunc ArrayCallback,
                                     AttrSetupUpcallFunc AttrCallback);

extern void SstFFSMarshal(SstStream Stream, void *Variable, const char *Name,
                          const char *Type, size_t ElemSize, size_t DimCount,
                          const size_t *Shape, const size_t *Count,
                          const size_t *Offsets, const void *data);
extern void SstFFSMarshalAttribute(SstStream Stream, const char *Name,
                                   const char *Type, size_t ElemSize,
                                   size_t ElemCount, const void *data);
extern void SstFFSGetDeferred(SstStream Stream, void *Variable,
                              const char *Name, size_t DimCount,
                              const size_t *Start, const size_t *Count,
                              void *Data);
extern void SstFFSGetLocalDeferred(SstStream Stream, void *Variable,
                                   const char *Name, size_t DimCount,
                                   const int BlockID, const size_t *Count,
                                   void *Data);

extern SstStatusValue SstFFSPerformGets(SstStream Stream);

extern int SstFFSWriterBeginStep(SstStream Stream, int mode,
                                 const float timeout_sec);
extern void SstFFSWriterEndStep(SstStream Stream, size_t Step);

/*
 *  General Operations
 */
extern void SstSetStatsSave(SstStream Stream, SstStats Save);

#include "sst_data.h"

#define SST_POSTFIX ".sst"

#ifdef __cplusplus
}
#endif

#endif /* SST_H_*/
