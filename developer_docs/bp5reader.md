# BP5 Metadata handling, reader-side focus

This document is to read in the context of [BP5 Metadata
Marshalling](bp5format.md), which covers metadata creation in BP5.

BP5 Metadata overall setup includes MetaMetaData (which is just an FFS
Format Block, essentially a marshalled version of the Metadata
FieldList created in [BP5 Metadata Marshalling](bp5format.md)) and
Metadata block itself (I.E. the result of FFSEncoding a the virtual
structure created in [BP5 Metadata Marshalling](bp5format.md)).

First, some FFS basics.  You'll notice that InstallMetaMetaData in
BP5Deserializer.cpp mostly just consists of some copying and a call to
load_external_format_FMcontext().  This just loads the format
information (I.E. the marshalled version of the Metadata FieldList)
into FFS.  This is a necessary first step for deserializing metadata,
however MetaMetaData is used for Attributes (at least for the original
version of attribute encoding where they were done with a custom
structure (fields named for the attributes) rather than a generic one
(attribute names in data).  Because of this we don't process
MetaMetaData on installation, but wait to see how it is used.

Next, lets look at the start of InstallMetaData.  This basically takes
in an encoded metadata block and does everything necessary to setup
newly read variables, etc.  The first part of this is "re-inflating"
the virtual metadata structure from its encoded form to something just
like it was on the writer, a C-style structure with pointers.  Note
that while these are all valid pointers, this is not a classic C
structure where each pointed-to entity is separately malloc'd.  That
would be terribly inefficient.  Instead FFS keeps this as a single
data block but with internal pointers.  Trying to free() them
individually would not go well.  Note that this is true whether we can
decode an incoming block *in-place* or not.

That may require some explanation.  FFS' goal is to efficiently move
pointer-based structures from one memory space to another.  In order
to do that, it doesn't do the classic thing, copying each field
individually into the encode buffer.  Instead it copies the base
structure into the encode buffer, followed by the things pointed to by
fields in the base structure, then recursively down the data structure
until everything is in the buffer.  As this happens, pointers in
copied structures are turned into the integer offset of the pointed-to
copy, and all copied structures are appropriately aligned within the
encode buffer so that hopefully when they "land" in the receiving
memory space they'll have an appropriate alignment on that processor
too.  However, this isn't always possible.  For example when
transferring from a 32-bit machine to a 64-bit, lots of things change
including the size of pointers and the required alignment of data
types.  FFS was designed for this situation, but heterogeneity isn't
what it used to be and a lot of that code hasn't been seriously
exercised in some time, which is why the FMlocalize_structs() call in
InstallMetaData() is commented out.  Normally that call would take the
FMformatList from the encoding host, "localize" it to be suitable for
the decoding host, and then FFS would take care of the unpleasant
details.  However at present the world is pretty uniformly 64-bit
little-endian and none of this should be necessary.  The
localize_structs is commented out because for some reason that I
couldn't quite work out, FFS still thought it was necessary and that
change was the easiest way to avoid the problem.  Should we support
32-bit architectures or this code survive to run on 128-bit
architectures, things will have to change.  As it is, we should always
be following the `FFSdecode_in_place_possible() == TRUE` code path.

Something useful to note: setting the environment variable
"BP5DumpMetadata" will cause the output of the raw incoming metadata
by the Deserializer.  This might be a little ugly, but it can be useful.  For example, this is the portion of output for the 'c32' variable in staging_common/TestCommonWrite:
```
BPG_8_12_c32 = 
  BPG_8_12_c32 = 
Dims = 1 ,BlockCount = 1 ,DBCount = 1 ,Shape = 0x11e817388 10 ,Count = 0x11e817390 10 ,Offset = 0x11e817398 0 ,DataBlockLocation = 0x11e8173a0 272 ,MinMax = NULL,  ,
,
```
You see the field name with the "BPG" prefix indicating a global
array, element size of 8, ADIOS type of 12 (maps to complex float),
and the actual variable name at the end.  Dims, BlockCount and DBCount
are all 1.  The Shape Count and Offset are arrays, so they are
represented by their base address (after decoding) followed by their
elements.  DataBlockLocation is similar, showing the datablock at
offset 272.  Finally there is no MinMax for complex, so that pointer
is NULL.

Lets step back a bit.  When using BP5, we expect the engine to provide
the Deserializer with all of the MetaMetaData, and then the Metadata
block from each rank.  For the BP5 file reader in random access mode,
we also expect to be given the Metadata blocks for ever step.  We
don't need all the MetaMetaData up front _per se_, but we have to have
it _before_ any MetaData block that it was associated with.  The
FFSdecode_* calls in InstallMetaData() produce what are essentially
copies of the metadata structure that was created in the writer, and
because we don't do the sort of metadata "merging" that BP3/4 did on
the writer side, we'll have a copy of the metadata from **EACH*
writer, and in BP5 file reader random access mode, also for each step.
**These C-style pointer-based data structures are the core of
in-memory BP5 metadata.** We don't do aggregation, turn variable count
arrays int std::vector-based structures or really anything like
that. Instead most everything in BP5Deserializer is just support for
accessing those data structures as they are.  NOTE: Remember that the
entry for an array variable in each of these blocks is a
`MetaArrayRec` as described in [BP5 Metadata
Marshalling](bp5format.md)).  That's a C structure with pointers to
the Shape, Count, Offsets, etc for all the blocks that were written on
that rank on that step.  The problem?  Each rank may have different
metadata structure and therefore the MetaArrayRec structure for
Variable X may live at a different offset in each MetaData block.  So
coming up with the right offset to find a variable's data given rank
and step is key to making this work.

The principal data structure that the BP5 deserializer maintains is
the BP5VarRec.  This is BP5's internal per-variable record and it
matches one-on-one with a Variable class object in the IO, except that
the BP5VarRec is persistent for the life of the BP5Deserializer
object, where the Variable object may be deleted and recreated on
every timestep in streaming mode.  Note that BP5 tries to be much more
careful than other engines about storing engine-specific information
in the shared IO and variable objects (I.E. it doesn't do it).  In
order to maintain this separation, the deserializer maintains two maps
with which it associates IO Variable objects with their persistent
BP5VarRec entries, `VarByName` and `VarByKey`, which are indexed by
the variable name and by the Variable instance address, respectively.
One of these calls is often the first call upon entry to the
deserializer's public methods.  Additionally, each BP5VarRec has a
VarNum field.  These numbers are assigned sequentially starting with 0
for the first Variable encountered when processing metadata.  The
VarNum is an important value used for indexing into various arrays.

The details of the BP5VarRec entries are in BP5Deserializer.h.  There
are too many entries to go through individually, but most are obvious
from code context, so here we'll focus on the creation and indexing
mechanisms that drive metadata use.  BP5VarRec entries are created
during parsing of MetaMetaData entries (FFS Formats), which happens
the first time we encounter a MetaData entry that was encoded with
that MetaMetaData (FFS Format).  This happens in the BuildControl()
routine which creates a ControlInfo struct for the MetaMetaData.  The
ControlInfo struct looks like this:
```
    struct ControlInfo
    {
        FMFormat Format;
        int ControlCount;
        struct ControlInfo *Next;
        std::vector<size_t> *MetaFieldOffset;
        std::vector<size_t> *CIVarIndex;
        struct ControlStruct Controls[1];
    };
```

The Format field is essentially the MetaMetaID and is what this is
indexed by.  I.E. when we get a new MetaData block, we determine it's
Format and look up the ControlInfo struct, which tells us everything
we need to know about the MetaData block without parsing it.  The
ControlCount is how many Variables are represented in this block and
the MetaFieldOffset gives us the starting offset of each one in the
MetaData block.  Recall from [BP5 Metadata Marshalling](bp5format.md),
that that's either the offset of the atomic value, or for arrays the
offset of the MetaArrayRec structure.  So, MetaFieldOffset[i] is the
offset of the i'th variable in this block.  But that `i` index is of
the variables that are actually in this block, and it may not
correspond to the VarNum of that variable (which as per above is
assigned the first time we see a Variable), to the CIVarIndex maps
from the VarNum index to the i'th entry in this block.

The Controls array is the per-variable entry in the ControlInfo struct
and it contains info directly parsed from the FMFieldList for this
entry plus a pointer to the VarRec that this is associated with:
```
    struct ControlStruct
    {
        int FieldOffset;
        BP5VarRec *VarRec;
        ShapeID OrigShapeID;
        DataType Type;
        int ElementSize;
    };
``
Please forgive the C-style structs and code.  Much of BP5 code was
derived from the C-based FFS marshaling method in SST.  Not everything
was converted to a more C++ style.

Now, lets first talk about the simple situation, non-random access
mode (I.E. step mode).  In this situation if we have N ranks in the
MPI cohort, we expect to have InstallMetaData() called for each one,
and the BP5Deserializer keeps a simple vector m_MetadataBaseAddrs
indexed by rank number.  Each VarRec also has a
PerWriterMetaFieldOffset array (filled in as we did each Install(), so
the address of the metadata for a particular variable from a
particular rank is basically
`m_MetadataAddrs[Rank]+VarRec->PerWriterMetaFieldOffset[Rank]`.
You'll see this code in BP5Deserializer::GetMetadataBase(), with the
added protection that if `VarRec->PerWriterMetaFieldOffset[Rank] ==
0`, that WriterRank didn't write that variable on that timestep.

Random access mode, where we have the metadata for a bunch of steps in
memory at the same time, is vastly more complex.  We didn't want to
mess up the speed and simplicity of the step-based mode, so this code
is split out in a separate `if` in most places, but lets step through
that branch in GetMetadataBase() because it hits on important points
in the BP5Deserializer code.  The first few lines of this branch are:
```
        if (Step >= m_ControlArray.size() || WriterRank >= m_ControlArray[Step].size())
        {
            return NULL; // we don't have this rank in this step
        }
```

These are bounds checks.  Like several other data structures in
BP5Deserializer, m_ControlArray is a vector of vectors.  The first
"dimension" here is the step, so the first predicate of this if checks
to see if the requested Step is larger than the size of m_ControlArray
which has entries for each step for which we have metadata.  If it is
larger, we've got no metadata and return NULL.  The second predicate
is maybe a little less obvious.  It turns out that the number of
writer ranks contributing to a BP5 file is not necessarily constant.
It is constant for a single write session, but you can close a BP5
file and reopen it in append mode with a different number of writers.
So the second "dimension" of the m_ControlArray is the number of
writer ranks that was in use for that step.  If we're asking for the
metadata for a writer rank that is larger that what was used for that
step, we don't have it and return NULL.

OK, the next bit:
```
        ControlInfo *CI = m_ControlArray[Step][WriterRank]; // writer control array
        if (((*CI->MetaFieldOffset).size() <= VarRec->VarNum) ||
            ((*CI->MetaFieldOffset)[VarRec->VarNum] == 0))
        {
            // Var does not appear in this record
            return NULL;
        }
```

`CI` here is the ControlInfo block for this WriterRank on this Step.
Like all FMFormats, it's really a template and lots of metadata blocks
likely have the same template, so this pointer is not unique, but it
is the template that applies to the metadata block for this Rank and
Step.  But we have a couple more checks.  MetaFieldOffset is indexed
by VarNum, and it's size corresponds to the highest VarNum we had seen
at the time that this CI was produced (I.E. the corresponding
MetaMetaData was parsed).  If the VarNum we're interested in is larger
than the MetaFieldOffset array, that Var was unknown when this was
parsed, therefore it's not in this CI.  On the other hand, if the
VarNum was known, but simply didn't appear in this CI, the
MetaFieldOffset is 0, and we also don't have metadata here.  (Note
that there are headers like the BitField that appear first in
metadata, so a zero offset is never valid for a Var field.)

OK, we've gotten to the point where we have a CI for this metadata
block and the template contains the variable we're interested in,
there's one more check:
```
        size_t CI_VarIndex = (*CI->CIVarIndex)[VarRec->VarNum];
        BP5MetadataInfoStruct *BaseData =
            (BP5MetadataInfoStruct *)(*MetadataBaseArray[Step])[WriterRank];
        if (!BP5BitfieldTest(BaseData, (int)CI_VarIndex))
        {
            // Var appears in CI, but wasn't written on this step
            return NULL;
        }
```

MetadataBaseArray, like m_ControlArray, is a vector of vectors, and it
contains a pointer to the metadata block for this rank/step (I.E. the
virtual structure that we build in [BP5 Metadata
Marshalling](bp5format.md).  We need to check to see if this variable,
while described in the MetaMetaData, was actually written on this
step, and to do that we have to check the bitfield.  Because the
bitfield is indexed not by VarNum but by the index of the Variable in
that block, we first have to lookup that index using the CIVarIndex
vector in the CI.  This is indexed by VarNum and maps it back to
CI_VarIndex.  Given that and the address of the metadatablock, we use
BP5BitfieldTest to see if the variable was actually written on this
step and return NULL if not.

Finally, we're done with checks and mapping.  The address of whatever
metadata is associated with this variable on this step and rank is the
base address of the metadata block plus the MetadataFieldOffset:
```
        size_t MetadataFieldOffset = (*CI->MetaFieldOffset)[VarRec->VarNum];
        writer_meta_base = (MetaArrayRec *)(((char *)(*MetadataBaseArray[Step])[WriterRank]) +
                                            MetadataFieldOffset);
```

GetMetadataBase() is the workhorse of BP5 reader-side metadata.  The
ReadRandomAccess code path may seem like a lot, but mostly the most
complex operations there are indexes into arrays and adding offsets.
It's got a lot of checks, but it runs pretty quick.

Most of the rest of BP5Deserializer is pretty straightforward if you
understand how GetMetadataBase() works, but there is one more
complexity that is somewhat the bane of BP5.  Every time we mention
"Step" in random access mode above, we mean an absolute step number.
That is, we start with 0 at writer's first Begin/EndStep and increment
by one on every subsequent Begin/EndStep (handling appending
appropriately by starting with the number of steps already in the
file).  However, many (all?)  things in ADIOS random access mode API
have traditionally been in terms of "relative" steps.  Relative steps
don't increment if the variable isn't written on that step.  So if you
write 10 steps into a file, but only write variable X on the even
absolute steps (0,2,4,6,8), then BP metadata must show 5 steps for
that variable and they should be steps 0-4 (_FOR THAT VARIABLE_).  So
if the user asks, for example, for the Shape of that variable on Step
4, we must internally map that RelativeStep specification to an
AbsoluteStep before applying the logic above.  We've tried to use the
variable name RelStep when dealing with a relative step spec, but
there's probably places that have been missed.  (Hopefully there's not
logic that has been missed too.)

# BP5 Read logic