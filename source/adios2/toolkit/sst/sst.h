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

typedef enum { SstSuccess, SstEndOfStream, SstFatalError } SstStatusValue;

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

/*
 *  Writer-side operations
 */
extern SstStream SstWriterOpen(const char *filename, SstParams Params,
                               MPI_Comm comm);

extern void SstStreamDestroy(SstStream Stream);

typedef void (*DataFreeFunc)(void *Data);
extern void SstProvideTimestep(SstStream s, SstData LocalMetadata,
                               SstData LocalData, long Timestep,
                               DataFreeFunc FreeData, void *FreeClientData);
extern void SstWriterClose(SstStream stream);

/*
 *  Reader-side operations
 */
extern SstStream SstReaderOpen(const char *filename, SstParams Params,
                               MPI_Comm comm);
extern void SstReaderGetParams(SstStream stream, int *WriterFFSmarshal,
                               int *WriterBPmarshal);
extern SstFullMetadata SstGetCurMetadata(SstStream stream);
extern void *SstReadRemoteMemory(SstStream s, int rank, long timestep,
                                 size_t offset, size_t length, void *buffer,
                                 void *DP_TimestepInfo);
extern SstStatusValue SstWaitForCompletion(SstStream stream, void *completion);
extern void SstReleaseStep(SstStream stream);
extern SstStatusValue SstAdvanceStep(SstStream stream, int mode,
                                     const float timeout_sec);
extern void SstReaderClose(SstStream stream);
extern long SstCurrentStep(SstStream s);

/*
 *  Calls that support FFS-based marshaling, source code in cp/ffs_marshal.c
 */
typedef void *(*VarSetupUpcallFunc)(void *Reader, const char *Name,
                                    const char *Type, void *Data);
typedef void *(*ArraySetupUpcallFunc)(void *Reader, const char *Name,
                                      const char *Type, int DimsCount,
                                      size_t *Shape, size_t *Start,
                                      size_t *Count);
extern void SstReaderInitFFSCallback(SstStream stream, void *Reader,
                                     VarSetupUpcallFunc VarCallback,
                                     ArraySetupUpcallFunc ArrayCallback);

extern void SstFFSMarshal(SstStream Stream, void *Variable, const char *Name,
                          const char *Type, size_t ElemSize, size_t DimCount,
                          const size_t *Shape, const size_t *Count,
                          const size_t *Offsets, const void *data);
extern void SstFFSGetDeferred(SstStream Stream, void *Variable,
                              const char *Name, size_t DimCount,
                              const size_t *Start, const size_t *Count,
                              void *Data);

extern void SstFFSPerformGets(SstStream Stream);

extern int SstFFSWriterBeginStep(SstStream Stream, int mode,
                                 const float timeout_sec);
extern void SstFFSWriterEndStep(SstStream Stream, size_t Step);

/*
 *  General Operations
 */
extern void SstSetStatsSave(SstStream Stream, SstStats Save);

#include "sst_data.h"

#define SST_POSTFIX ".sst"

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

#ifdef __cplusplus
}
#endif

#endif /* SST_H_*/
