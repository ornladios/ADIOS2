SstMetadata CreateDummyMetadata(long TimeStep, int Rank, int Size,
                                int DataSize);

extern SstData CreateDummyData(long TimeStep, int Rank, int Size, int DataSize);

extern int ValidateDummyData(long TimeStep, int Rank, int Size, int Offset,
                             void *Buffer, int DataSize);
