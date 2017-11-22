extern void adios_sst_write(SstStream s, void *BaseType, int BaseSize,
                            int DimCount, struct _SstDimenMeta *Dims,
                            char *Name, char *Path, void *Data)
{
    M = s->CurMetaData;
    D = s->CurData;
    if (!Dims)
    {
        /* atomic data value */
        if (BaseType == SST_FLOAT)
        {
            M->FloatVars = realloc(M->FloatVars, sizeof(M->FloatVars[0]) *
                                                     (M->FloatVarCount + 1));
            M->FloatVars[M->FloatVarCount].VarName = strdup(Name);
            if (BaseSize == sizeof(float))
            {
                M->FloatVars[M->FloatVarCount].Value = *((float *)Data);
            }
            else if (BaseSize == sizeof(double))
            {
                M->FloatVars[M->FloatVarCount].Value = *((double *)Data);
            }
            else
            {
                assert(FALSE);
            }
            M->FloatVarCount++;
        }
        else if (BaseType == SST_INT)
        {
            M->IntVars = realloc(M->IntVars,
                                 sizeof(M->IntVars[0]) * (M->IntVarCount + 1));
            M->IntVars[M->IntVarCount].VarName = strdup(Name);
            if (BaseSize == sizeof(char))
            {
                M->IntVars[M->IntVarCount].Value = *((char *)Data);
            }
            else if (BaseSize == sizeof(short))
            {
                M->IntVars[M->IntVarCount].Value = *((short *)Data);
            }
            else if (BaseSize == sizeof(int))
            {
                M->IntVars[M->IntVarCount].Value = *((int *)Data);
            }
            else if (BaseSize == sizeof(int64_t))
            {
                M->IntVars[M->IntVarCount].Value = *((int64_t *)Data);
            }
            else
            {
                assert(FALSE);
            }
            M->IntVarCount++;
        }
        else if (BaseType == SST_UINT)
        {
            M->UintVars = realloc(M->UintVars, sizeof(M->UintVars[0]) *
                                                   (M->UintVarCount + 1));
            M->UintVars[M->UintVarCount].VarName = strdup(Name);
            if (BaseSize == sizeof(char))
            {
                M->UintVars[M->UintVarCount].Value = *((unsigned char *)Data);
            }
            else if (BaseSize == sizeof(short))
            {
                M->UintVars[M->UintVarCount].Value = *((unsigned short *)Data);
            }
            else if (BaseSize == sizeof(int))
            {
                M->UintVars[M->UintVarCount].Value = *((unsigned int *)Data);
            }
            else if (BaseSize == sizeof(int64_t))
            {
                M->UintVars[M->UintVarCount].Value = *((uint64_t *)Data);
            }
            else
            {
                assert(FALSE);
            }
            M->UintVarCount++;
        }
        else
        {
            /* handle user-specified structure */
            /* not yet */
        }
    }
    else
    {
        /* global or local array, store in data block */
        int BlockLength = BaseSize;
        M->Vars = realloc(M->Vars, sizeof(M->Vars[0]) * (M->VarCount + 1));
        M->Vars[M->VarCount].VarName = strdup(Name);
        M->Vars[M->VarCount].DimensionCount = DimCount;
        M->Vars[M->VarCount].Dimensions =
            malloc(DimCount * sizeof(Dimensions[0]));
        memcpy(M->Vars[M->VarCount].Dimensions, Dimensions,
               DimCount * sizeof(Dimensions[0]));
        for (int i = 0; i < DimCount; i++)
        {
            BlockLength *= Dimensions[i].Size;
        }
        D->block = realloc(D->block, D->DataSize + BlockLength);
        memcpy(&D->block[D->DataSize], Data, BlockLength);
        M->Vars[M->VarCount].DataOffsetInBlock = D->DataSize;
        D->DataSize += BlockLength;
        M->VarCount++;
    }
}
