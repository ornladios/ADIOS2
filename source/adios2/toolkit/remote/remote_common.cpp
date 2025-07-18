#include "adios2/common/ADIOSConfig.h"

#include "remote_common.h"
#include <evpath.h>

namespace adios2
{
namespace EVPathRemoteCommon
{

FMField OpenFileList[] = {
    {"OpenResponseCondition", "integer", sizeof(int), FMOffset(OpenFileMsg, OpenResponseCondition)},
    {"FileName", "string", sizeof(char *), FMOffset(OpenFileMsg, FileName)},
    {"Mode", "integer", sizeof(RemoteFileMode), FMOffset(OpenFileMsg, Mode)},
    {"RowMajorOrder", "integer", sizeof(int), FMOffset(OpenFileMsg, RowMajorOrder)},
    {NULL, NULL, 0, 0}};

FMStructDescRec OpenFileStructs[] = {{"OpenFile", OpenFileList, sizeof(struct _OpenFileMsg), NULL},
                                     {NULL, NULL, 0, NULL}};

FMField OpenSimpleFileList[] = {
    {"OpenResponseCondition", "integer", sizeof(int),
     FMOffset(OpenSimpleFileMsg, OpenResponseCondition)},
    {"FileName", "string", sizeof(char *), FMOffset(OpenSimpleFileMsg, FileName)},
    {"ReadContents(0)", "integer", sizeof(long), FMOffset(OpenSimpleFileMsg, ReadContents)},
    {NULL, NULL, 0, 0}};

FMStructDescRec OpenSimpleFileStructs[] = {
    {"OpenSimpleFile", OpenSimpleFileList, sizeof(struct _OpenSimpleFileMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField OpenResponseList[] = {
    {"OpenResponseCondition", "integer", sizeof(int),
     FMOffset(OpenResponseMsg, OpenResponseCondition)},
    {"FileHandle", "integer", sizeof(intptr_t), FMOffset(OpenResponseMsg, FileHandle)},
    {NULL, NULL, 0, 0}};

FMStructDescRec OpenResponseStructs[] = {
    {"OpenResponse", OpenResponseList, sizeof(struct _OpenResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField OpenSimpleResponseList[] = {
    {"OpenResponseCondition", "integer", sizeof(int),
     FMOffset(OpenSimpleResponseMsg, OpenResponseCondition)},
    {"FileHandle", "integer", sizeof(intptr_t), FMOffset(OpenSimpleResponseMsg, FileHandle)},
    {"FileSize", "integer", sizeof(size_t), FMOffset(OpenSimpleResponseMsg, FileSize)},
    {"FileContents", "char[FileSize]", sizeof(char), FMOffset(OpenSimpleResponseMsg, FileContents)},
    {NULL, NULL, 0, 0}};

FMStructDescRec OpenSimpleResponseStructs[] = {
    {"OpenSimpleResponse", OpenSimpleResponseList, sizeof(struct _OpenSimpleResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField GetRequestList[] = {
    {"GetResponseCondition", "integer", sizeof(int), FMOffset(GetRequestMsg, GetResponseCondition)},
    {"FileHandle", "integer", sizeof(int64_t), FMOffset(GetRequestMsg, FileHandle)},
    {"RequestType", "integer", sizeof(int), FMOffset(GetRequestMsg, RequestType)},
    {"Step", "integer", sizeof(size_t), FMOffset(GetRequestMsg, Step)},
    {"StepCount", "integer", sizeof(size_t), FMOffset(GetRequestMsg, StepCount)},
    {"VarName", "string", sizeof(char *), FMOffset(GetRequestMsg, VarName)},
    {"BlockID", "integer", sizeof(int64_t), FMOffset(GetRequestMsg, BlockID)},
    {"DimCount", "integer", sizeof(size_t), FMOffset(GetRequestMsg, DimCount)},
    {"Count", "integer[DimCount]", sizeof(size_t), FMOffset(GetRequestMsg, Count)},
    {"Start", "integer[DimCount]", sizeof(size_t), FMOffset(GetRequestMsg, Start)},
    {"Error", "double", sizeof(double), FMOffset(GetRequestMsg, Error)},
    {"Norm", "double", sizeof(double), FMOffset(GetRequestMsg, Norm)},
    {"Relative", "integer", sizeof(uint8_t), FMOffset(GetRequestMsg, Relative)},
    {"Dest", "integer", sizeof(size_t), FMOffset(GetRequestMsg, Dest)},
    {NULL, NULL, 0, 0}};

FMStructDescRec GetRequestStructs[] = {{"Get", GetRequestList, sizeof(struct _GetRequestMsg), NULL},
                                       {NULL, NULL, 0, NULL}};

FMField ReadRequestList[] = {
    {"ReadResponseCondition", "integer", sizeof(int),
     FMOffset(ReadRequestMsg, ReadResponseCondition)},
    {"FileHandle", "integer", sizeof(intptr_t), FMOffset(ReadRequestMsg, FileHandle)},
    {"Offset", "integer", sizeof(size_t), FMOffset(ReadRequestMsg, Offset)},
    {"Size", "integer", sizeof(size_t), FMOffset(ReadRequestMsg, Size)},
    {"Dest", "integer", sizeof(void *), FMOffset(ReadRequestMsg, Dest)},
    {NULL, NULL, 0, 0}};

FMStructDescRec ReadRequestStructs[] = {
    {"Read", ReadRequestList, sizeof(struct _ReadRequestMsg), NULL}, {NULL, NULL, 0, NULL}};

FMField ReadResponseList[] = {
    {"ReadResponseCondition", "integer", sizeof(int),
     FMOffset(ReadResponseMsg, ReadResponseCondition)},
    {"Dest", "integer", sizeof(void *), FMOffset(ReadResponseMsg, Dest)},
    {"OperatorType", "integer", sizeof(uint8_t), FMOffset(ReadResponseMsg, OperatorType)},
    {"Size", "integer", sizeof(size_t), FMOffset(ReadResponseMsg, Size)},
    {"ReadData", "char[Size]", sizeof(char), FMOffset(ReadResponseMsg, ReadData)},
    {NULL, NULL, 0, 0}};

FMStructDescRec ReadResponseStructs[] = {
    {"ReadResponse", ReadResponseList, sizeof(struct _ReadResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField CloseFileList[] = {
    {"FileHandle", "integer", sizeof(intptr_t), FMOffset(CloseFileMsg, FileHandle)},
    {"CloseResponseCondition", "integer", sizeof(int),
     FMOffset(CloseFileMsg, CloseResponseCondition)},
    {NULL, NULL, 0, 0}};

FMStructDescRec CloseFileStructs[] = {{"Close", CloseFileList, sizeof(struct _CloseFileMsg), NULL},
                                      {NULL, NULL, 0, NULL}};

FMField CloseResponseList[] = {
    {"CloseResponseCondition", "integer", sizeof(int),
     FMOffset(CloseFileResponseMsg, CloseResponseCondition)},
    {"Status", "integer", sizeof(int), FMOffset(CloseFileResponseMsg, Status)},
    {"unused1", "integer", sizeof(size_t), FMOffset(CloseFileResponseMsg, unused)},
    {"unused2", "integer", sizeof(size_t), FMOffset(CloseFileResponseMsg, unused2)},
    {NULL, NULL, 0, 0}};

FMStructDescRec CloseResponseStructs[] = {
    {"CloseResponse", CloseResponseList, sizeof(struct _CloseFileResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField KillServerList[] = {{"KillResponseCondition", "integer", sizeof(long),
                             FMOffset(KillServerMsg, KillResponseCondition)},
                            {NULL, NULL, 0, 0}};

FMStructDescRec KillServerStructs[] = {
    {"KillServer", KillServerList, sizeof(struct _KillServerMsg), NULL}, {NULL, NULL, 0, NULL}};

FMField KillResponseList[] = {
    {"KillResponseCondition", "integer", sizeof(long),
     FMOffset(KillResponseMsg, KillResponseCondition)},
    {"Status", "string", sizeof(char *), FMOffset(KillResponseMsg, Status)},
    {NULL, NULL, 0, 0}};

FMStructDescRec KillResponseStructs[] = {
    {"KillResponse", KillResponseList, sizeof(struct _KillResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField StatusServerList[] = {{"StatusResponseCondition", "integer", sizeof(long),
                               FMOffset(StatusServerMsg, StatusResponseCondition)},
                              {NULL, NULL, 0, 0}};

FMStructDescRec StatusServerStructs[] = {
    {"StatusServer", StatusServerList, sizeof(struct _StatusServerMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField StatusResponseList[] = {
    {"StatusResponseCondition", "integer", sizeof(long),
     FMOffset(StatusResponseMsg, StatusResponseCondition)},
    {"Hostname", "string", sizeof(char *), FMOffset(StatusResponseMsg, Hostname)},
    {"Status", "string", sizeof(char *), FMOffset(StatusResponseMsg, Status)},
    {NULL, NULL, 0, 0}};

FMStructDescRec StatusResponseStructs[] = {
    {"StatusResponse", StatusResponseList, sizeof(struct _StatusResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

void RegisterFormats(EVPathRemoteCommon::Remote_evpath_state &ev_state)
{
    ev_state.OpenFileFormat = CMregister_format(ev_state.cm, EVPathRemoteCommon::OpenFileStructs);
    ev_state.OpenSimpleFileFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::OpenSimpleFileStructs);
    ev_state.OpenResponseFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::OpenResponseStructs);
    ev_state.OpenSimpleResponseFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::OpenSimpleResponseStructs);
    ev_state.GetRequestFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::GetRequestStructs);
    ev_state.ReadRequestFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::ReadRequestStructs);
    ev_state.ReadResponseFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::ReadResponseStructs);
    ev_state.CloseFileFormat = CMregister_format(ev_state.cm, EVPathRemoteCommon::CloseFileStructs);
    ev_state.CloseResponseFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::CloseResponseStructs);
    ev_state.KillServerFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::KillServerStructs);
    ev_state.KillResponseFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::KillResponseStructs);
    ev_state.StatusServerFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::StatusServerStructs);
    ev_state.StatusResponseFormat =
        CMregister_format(ev_state.cm, EVPathRemoteCommon::StatusResponseStructs);
}
}
}
