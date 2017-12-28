#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpi.h"

#include "sst.h"
#include "dummy.h"

SstMetadata CreateDummyMetadata(long TimeStep, int Rank, int Size, int DataSize)
{
    SstMetadata Meta = malloc(sizeof(struct _SstMetadata));
    struct _SstVarMeta *Var;
    Meta->DataSize = DataSize;
    Meta->VarCount = 1;
    Var = (struct _SstVarMeta *)malloc(sizeof(struct _SstVarMeta));
    Var->VarName = strdup("Array");
    Var->DimensionCount = 1;
    Var->Dimensions = malloc(sizeof(struct _SstDimenMeta));
    Var->Dimensions->Offset = Rank * DataSize;
    Var->Dimensions->Size = DataSize;
    Var->Dimensions->GlobalSize = DataSize * Size;
    Meta->Vars = Var;
    return Meta;
}

SstData CreateDummyData(long TimeStep, int Rank, int Size, int DataSize)
{
    SstData Data = malloc(sizeof(struct _SstData));
    Data->DataSize = DataSize;
    Data->block = malloc(DataSize);
    double *Tmp = (double *)Data->block;
    for (int i = 0; i < DataSize / 8; i++) {
        Tmp[i] = TimeStep * 1000.0 + Rank * 10.0 + i;
    }
    return Data;
}

extern int ValidateDummyData(long TimeStep, int Rank, int Size, int offset,
                             void *buffer, int DataSize)
{
    int Start = (offset + 7) / 8;
    int Remainder = offset % 8;
    double FirstDouble = TimeStep * 1000.0 + Rank * 10.0 + (Start - 1);
    double *Tmp = (double *)((char *)buffer + 8 - Remainder);
    int Result = 0;

    if (Remainder != 0) {
        Result =
            memcmp(buffer, ((char *)&FirstDouble) + Remainder, 8 - Remainder);
    } else {
        Tmp = buffer;
    }

    for (int i = Start; i < DataSize / 8; i++) {
        Result |= !(*Tmp == TimeStep * 1000.0 + Rank * 10.0 + i);
        Tmp++;
    }
    return Result;
}
