#include "evpath.h"
#include <stddef.h>

namespace adios2
{
namespace RemoteCommon
{

enum REVPFileMode
{
    REVPOpen,
    REVPOpenReadComplete,
    REVPWrite
};
/*
 */
typedef struct _OpenFileMsg
{
    int OpenResponseCondition;
    char *FileName;
    REVPFileMode Mode;
} * OpenFileMsg;

typedef struct _OpenResponseMsg
{
    int OpenResponseCondition;
    int64_t FileHandle;
    size_t FileSize;    // may be used for Contents size
    char *FileContents; // used for OpenReadComplete mode
} * OpenResponseMsg;

/*
 */
typedef struct _GetRequestMsg
{
    int GetResponseCondition;
    int RequestType;
    int64_t FileHandle;
    char *VarName;
    size_t Step;
    int DimCount;
    size_t *Count;
    size_t *Start;
    void *Dest;
} * GetRequestMsg;

/*
 */
typedef struct _ReadRequestMsg
{
    int ReadResponseCondition;
    int64_t FileHandle;
    size_t Offset;
    size_t Size;
    void *Dest;
} * ReadRequestMsg;

/*
 * Reader register messages are sent from reader rank 0 to writer rank 0
 * They contain basic info, plus contact information for each reader rank
 */
typedef struct _ReadResponseMsg
{
    int ReadResponseCondition;
    void *Dest;
    size_t Size;
    char *ReadData;
} * ReadResponseMsg;

/*
 */
typedef struct _CloseFileMsg
{
    void *FileHandle;
} * CloseFileMsg;

enum VerbosityLevel
{
    NoVerbose = 0,       // Generally no output (but not absolutely quiet?)
    CriticalVerbose = 1, // Informational output for failures only
    SummaryVerbose =
        2, // One-time summary output containing general info (transports used,
           // timestep count, stream duration, etc.)
    PerStepVerbose = 3, // One-per-step info, generally from rank 0 (metadata
                        // read, Begin/EndStep verbosity, etc.)
    PerRankVerbose = 4, // Per-step info from each rank (for those things that
                        // might be different per rank).
    TraceVerbose = 5,   // All debugging available
};

struct Remote_evpath_state
{
    CManager cm;
    CMFormat OpenFileFormat;
    CMFormat OpenResponseFormat;
    CMFormat GetRequestFormat;
    CMFormat ReadRequestFormat;
    CMFormat ReadResponseFormat;
    CMFormat CloseFileFormat;
};

void RegisterFormats(struct Remote_evpath_state &ev_state);

}; // end of namespace remote_common
}; // end of namespace adios2
