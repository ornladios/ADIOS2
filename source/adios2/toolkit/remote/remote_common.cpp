#include "remote_common.h"
#include <evpath.h>

namespace adios2
{
namespace RemoteCommon
{

FMField OpenFileList[] = {
    {"OpenResponseCondition", "integer", sizeof(long),
     FMOffset(OpenFileMsg, OpenResponseCondition)},
    {"FileName", "string", sizeof(char *), FMOffset(OpenFileMsg, FileName)},
    {"Mode", "integer", sizeof(REVPFileMode), FMOffset(OpenFileMsg, Mode)},
    {NULL, NULL, 0, 0}};

FMStructDescRec OpenFileStructs[] = {
    {"OpenFile", OpenFileList, sizeof(struct _OpenFileMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField OpenResponseList[] = {
    {"OpenResponseCondition", "integer", sizeof(long),
     FMOffset(OpenResponseMsg, OpenResponseCondition)},
    {"FileHandle", "integer", sizeof(intptr_t),
     FMOffset(OpenResponseMsg, FileHandle)},
    {"FileSize", "integer", sizeof(size_t),
     FMOffset(OpenResponseMsg, FileSize)},
    {"FileContents", "char[FileSize]", sizeof(char),
     FMOffset(OpenResponseMsg, FileContents)},
    {NULL, NULL, 0, 0}};

FMStructDescRec OpenResponseStructs[] = {
    {"OpenResponse", OpenResponseList, sizeof(struct _OpenResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField GetRequestList[] = {
    {"GetResponseCondition", "integer", sizeof(int),
     FMOffset(GetRequestMsg, GetResponseCondition)},
    {"FileHandle", "integer", sizeof(int64_t),
     FMOffset(GetRequestMsg, FileHandle)},
    {"RequestType", "integer", sizeof(int),
     FMOffset(GetRequestMsg, RequestType)},
    {"Step", "integer", sizeof(size_t), FMOffset(GetRequestMsg, Step)},
    {"VarName", "string", sizeof(char *), FMOffset(GetRequestMsg, VarName)},
    {"DimCount", "integer", sizeof(size_t), FMOffset(GetRequestMsg, DimCount)},
    {"Count", "integer[DimCount]", sizeof(size_t),
     FMOffset(GetRequestMsg, Count)},
    {"Start", "integer[DimCount]", sizeof(size_t),
     FMOffset(GetRequestMsg, Start)},
    {"Dest", "integer", sizeof(size_t), FMOffset(GetRequestMsg, Dest)},
    {NULL, NULL, 0, 0}};

FMStructDescRec GetRequestStructs[] = {
    {"Get", GetRequestList, sizeof(struct _GetRequestMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField ReadRequestList[] = {
    {"ReadResponseCondition", "integer", sizeof(long),
     FMOffset(ReadRequestMsg, ReadResponseCondition)},
    {"FileHandle", "integer", sizeof(intptr_t),
     FMOffset(ReadRequestMsg, FileHandle)},
    {"Offset", "integer", sizeof(size_t), FMOffset(ReadRequestMsg, Offset)},
    {"Size", "integer", sizeof(size_t), FMOffset(ReadRequestMsg, Size)},
    {"Dest", "integer", sizeof(void *), FMOffset(ReadRequestMsg, Dest)},
    {NULL, NULL, 0, 0}};

FMStructDescRec ReadRequestStructs[] = {
    {"Read", ReadRequestList, sizeof(struct _ReadRequestMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField ReadResponseList[] = {
    {"ReadResponseCondition", "integer", sizeof(long),
     FMOffset(ReadResponseMsg, ReadResponseCondition)},
    {"Dest", "integer", sizeof(void *), FMOffset(ReadResponseMsg, Dest)},
    {"Size", "integer", sizeof(size_t), FMOffset(ReadResponseMsg, Size)},
    {"ReadData", "char[Size]", sizeof(char),
     FMOffset(ReadResponseMsg, ReadData)},
    {NULL, NULL, 0, 0}};

FMStructDescRec ReadResponseStructs[] = {
    {"ReadResponse", ReadResponseList, sizeof(struct _ReadResponseMsg), NULL},
    {NULL, NULL, 0, NULL}};

FMField CloseFileList[] = {{"FileHandle", "integer", sizeof(intptr_t),
                            FMOffset(CloseFileMsg, FileHandle)},
                           {NULL, NULL, 0, 0}};

FMStructDescRec CloseFileStructs[] = {
    {"Close", CloseFileList, sizeof(struct _CloseFileMsg), NULL},
    {NULL, NULL, 0, NULL}};

void RegisterFormats(RemoteCommon::Remote_evpath_state &ev_state)
{
    ev_state.OpenFileFormat =
        CMregister_format(ev_state.cm, RemoteCommon::OpenFileStructs);
    ev_state.OpenResponseFormat =
        CMregister_format(ev_state.cm, RemoteCommon::OpenResponseStructs);
    ev_state.GetRequestFormat =
        CMregister_format(ev_state.cm, RemoteCommon::GetRequestStructs);
    ev_state.ReadRequestFormat =
        CMregister_format(ev_state.cm, RemoteCommon::ReadRequestStructs);
    ev_state.ReadResponseFormat =
        CMregister_format(ev_state.cm, RemoteCommon::ReadResponseStructs);
    ev_state.CloseFileFormat =
        CMregister_format(ev_state.cm, RemoteCommon::CloseFileStructs);
}
}
}
