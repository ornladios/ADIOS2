# BP5 Metadata Marshaling, writer-side focus

BP5 Metadata Marshalling is based upon FFS, which provides the ability
to serialize a C-style pointer-based data structure (starting with a
base struct) and to deserialize it in-place on the receiving side.
This is what we'll do to encode BP5 Metadata, create a custom C-style
struct on the writer side and then use FFS to make that same struct
available to the reader.

Normally, in order to use FFS, an application must fully describe
the base structure using an FMFieldList, where each element
describes a field in the structure, including the field's name,
basic type (integer, float, etc.), size and offset from the start
of the structure.  In "normal" scenarios, like in SST this is
straightforward because we're describing a structure that exists
at compile-time and all of those things are compile-time static.
However, ADIOS metadata represents information about variables
that we don't know about until run-time, so if we're going to use
FFS here, things have to be a bit more dynamic.  In particular,
we'll represent ADIOS metadata with a "virtual" structure, one
whose description we'll construct on the fly and which will only
ever exist virtually, making up offsets as we go.  We just have to
be careful about keeping things aligned appropriately because we
want this to land on the receiver and be appropriately aligned
there.  (Normally the compiler takes care of this, but this
virtual structure is never seen by a compiler, so we're doing it.)
The field name that we specify to FFS is also important because we
use it to communicate a lot of information between writer and
reader.  While it always contains the variable name, it also
encodes the variable type (local or global, atomic or array,
compressed, derived, etc.).  Because the variable name only
appears in the metametadata (ffs format), this is a great place to
put more static information about the variable, specifically
anything that is fixed after definition and doesn't change on a
per-timestep basis.  More on names later.

To accomplish managing the structure on the writer side, we
principally track two things, the FMFieldList that represents the
description of the virtual struct, and a malloc'd region where we
build the virtual struct itself.  While the description is
interpreted by FFS, the most important thing for BP5 to remember
is this field's offset because that's where the (meta)data will
go.  When we Marshal a simple atomic value (local or global), we
calculate an appropriately aligned new offset in the buffer, add
to the FMFieldList (maintained in Info.MetaFields on the writer)
and copy the data into the virtual field at that offset in the
buffer.  On future timesteps, the field already exists, so we just
use the offset and copy the data into the buffer.  Arrays are a
bit more complex, but lets start with the simple case.  FFS
supports substructures, I.E. fields which themselves are a
structure and we use that feature for all array representations.
There are several things that may change on a per-timestep basis
for arrays, including Shape, Count and Offset values (which are
themselves arrays), and we also need to track the location of the
related data block (offset in this rank's data segment).  Except
for Shape (which we assume is set for at least this timestep), all
of these things are per-block.

Back to FFS capabilities for a moment.  FFS's pointer-based
structures include dynamically-sized arrays, and the size of those
arrays must be specified by an integer-typed field in that
structure.  There are three different array lengths required here.
Shape is of length Dims (how many dimensions the array has),
DataBlockLocation is of length BlockCount (how many blocks were
written on this rank), and for Count and Offsets we must have
those per-block, so the length is Dims*BlockCount.  To satisfy
FFS's constraints, that means we must have integer fields
representing all three lengths in the array metadata struct, and
we need pointers to the dynamic arrays representing Shape, Count,
Offsets, and DataBlockLocation.  These are the BASE_FIELDS below
and the FFS FMField entries are BASE_FIELD_ENTRIES in BP5Base.cpp.
```
#define BASE_FIELDS                                                                                \
    size_t Dims;               /* How many dimensions does this array have */                      \
    size_t BlockCount;         /* How many blocks are written   */                                 \
    size_t DBCount;            /* Dimens * BlockCount   */                                         \
    size_t *Shape;             /* Global dimensionality  [Dims] NULL for local */                  \
    size_t *Count;             /* Per-block Counts    [DBCount] */                                 \
    size_t *Offsets;           /* Per-block Offsets   [DBCount] NULL for local */                  \
    size_t *DataBlockLocation; /* Per-block Offset in PG [BlockCount] */
```
```
#define BASE_FIELD_ENTRIES                                                                         \
    {"Dims", "integer", sizeof(size_t), FMOffset(BP5Base::MetaArrayRec *, Dims)},                  \
        {"BlockCount", "integer", sizeof(size_t), FMOffset(BP5Base::MetaArrayRec *, BlockCount)},  \
        {"DBCount", "integer", sizeof(size_t), FMOffset(BP5Base::MetaArrayRec *, DBCount)},        \
        {"Shape", "integer[Dims]", sizeof(size_t), FMOffset(BP5Base::MetaArrayRec *, Shape)},      \
        {"Count", "integer[DBCount]", sizeof(size_t), FMOffset(BP5Base::MetaArrayRec *, Count)},   \
        {"Offset", "integer[DBCount]", sizeof(size_t),                                             \
         FMOffset(BP5Base::MetaArrayRec *, Offsets)},                                              \
        {"DataBlockLocation", "integer[BlockCount]", sizeof(size_t),                               \
         FMOffset(BP5Base::MetaArrayRec *, DataBlockLocation)},
```
While more complex arrays metadata entries are necessary, these
must be the first fields in those structures.  While there can't
be a static struct declaration for all of the metadata, there is a
static declaration for the array metadata substructure,
`MetaArrayRec` below.
```
    typedef struct _MetaArrayRec
    {
        BASE_FIELDS
    } MetaArrayRec;
```
Mostly you'll see this used like this:
```
MetaArrayRec *MetaEntry = (MetaArrayRec *)((char *)(MetadataBuf) +  Rec->MetaOffset);
```
This gives us a nice way of accessing the key fields in an array's
metadata entry.

So, what about more complex arrays?  All of our compression
operators require the length of the encrypted field as input to
the uncompress operator.  Generally we don't include data block
length as part of metadata because it's easily calculated from the
Count values and the length of the data type, but in order to
support compression we have to communicate it from the writer to
the reader so we can uncompress.  Therefore every field with an
operator has as its next field (after BASE_FIELDS) DataBlockSize.
Like DataBlockLocation, this is per block (and so it's FFS
description also uses BlockCount).  This arrangement is
represented by the `struct MetaArrayRecOperator` below.  Note that
BP5 does not itself use the DataBlockSize in the metadata.  The
size of the compressed data is returned from the compression
operator, and is used by BP5 to copy that data into the data
block, but after that it is only passed to the Uncompress operator
on the receiving side, so operators like MGard may choose to use
this differently.
```
    typedef struct _MetaArrayRecOperator
    {
        BASE_FIELDS
        size_t *DataBlockSize; // Per-block Lengths [BlockCount]
    } MetaArrayRecOperator;
```
The last case is arrays that also have Min/Max stats associated
with them.  Since this can be combined with operators, that gives
us two more possible structs for array metadata, a plain array
with Min/Max or an array with an operator and Min/Max, these are
represented by the structs `MetaArrayRecMM` and
`MetaArrayRecOperatorMM` below.  Note that MinMax in that struct is
a `char*`, but obviously the data type of Min/Max depends upon the
element type of the array.  How does that work?  The actual size
in bytes of the MinMax array is `BlockCount * sizeof(array element) * 2`, but in order to avoid introducing yet another integer-typed
size value into the structure we've gone to some effort in order
to leverage the existing BlockCount value.  In particular, there
are a number of FMField lists for The MM and OperatorMM arrays,
each giving FFS a different element size for the MinMax Array.
ADIOS types of size 1 use `MetarrayRecMM1List`, those of size 2 use
`MetaArrayRecMM2List`, etc., up to `MetaArrayRecMM16List`, which would
be used by long double.  Note that BP5 doesn't define or support
MinMax for string, complex, or structure types.
```
    typedef struct _MetaArrayRecMM
    {
        BASE_FIELDS
        char *MinMax; // char[TYPESIZE][BlockCount]  varies by type
    } MetaArrayRecMM;

    typedef struct _MetaArrayRecOperatorMM
    {
        BASE_FIELDS
        size_t *DataBlockSize; // Per-block Lengths [BlockCount]
        char *MinMax;          // char[TYPESIZE][BlockCount]  varies by type
    } MetaArrayRecOperatorMM;
```
For each of the array variations above, when we add the field
associated with that array to the metadata field list, we specify
the appropriate FieldList in the FFS "field_type" value, and
allocate space for the relevant structure in the virtual metadata
struct we're building. (Example MetaArrayRecOperatorMM8List below.)
```
static FMField MetaArrayRecOperatorMM8List[] = {
    BASE_FIELD_ENTRIES
    {"DataBlockSize", "integer[BlockCount]", sizeof(size_t),
                       FMOffset(BP5Base::MetaArrayRecOperator *, DataBlockSize)},
    {"MinMax", "char[16][BlockCount]", 1, FMOffset(BP5Base::MetaArrayRecOperatorMM *, MinMax)},
    {NULL, NULL, 0, 0}};
```
We mentioned field names above, we actually encode a lot of
information into the FFS field names, including the variable name,
shape, element_size, ADIOS type, any operator that might be
applied, the name of the substructure (if the array is a struct
type), and even the expression that is to be used for derived
variables.  These are all encoded in different ways, for example
the basic shape of the variable is encoded in the three letter
prefix of the FFS fieldname: GlobalValue: = "BPg", GlobalArray =
"BPG"JoinedArray = "BPJ", LocalValue = "BPl", LocalArray = "BPL".
The details of the encoding are buried in the logic, but important
bit is knowing that there's a lot of information there and some of
it (like the expression) is base64 encoded to avoid having special
characters in the FFS field name.  From the BP5 point of view,
anything that can be encoded in the field name is a good thing
because it travels in the metametadata, not the metadata, so it
only gets moved around if the field set changes.

Speaking of changes, there are some details that are omitted above
to get the main points across, but lets talk about other details.
First, when you put a first block of an array, we fill out the
Dims field, init BlockCount to 1, DBCount (the `Dims*BlockCount`
value) to Dims and then we malloc memory to hold a copy of the
Shape, Count and Offset values.  (We need to copy these anyway as
part of serialization as they must be captured at the time of Put,
so we can't, say, just reference the values in the VariableBase
class.)  For LocalArrays, the Shape value stays at a NULL pointer,
as does the Start value.  If after the first there's another Put()
on that variable, we add 1 to BlockCount, increment DBCount by
Dims, and realloc() the Count and Offset arrays so that we can add
the new Count and Offset values after the ones that are already
there.  This means that the Count values for block 1 start at
`Count[Dims]`, for block 2 they start at `Count[2*Dims]`, etc.  At the
end of the timestep after using FFSencode() to serialize the
metadata, `FMfree_var_rec_elements()` is used to free() all these
subarrays that we've malloc'd.  It understands the structure of
our entire Metadata structure, walks the field list and
deallocates appropriately.  Once this has been done, we can
memset() the whole metadata structure back to zeros and we're
ready to start again.  (All pointers NULL and counts are zero.)

When we do start again with the next timestep, we don't start from
scratch with a new Fieldlist and virtual structure, but instead
try to reuse the old one.  The anticipation is that step-based HPC
applications are highly regular and the set of variables that are
output on step N+1 are likely the same as what they output for
step N.  So when we get a Put() for a variable, we look up its
entry in internal bookkeeping and if it has an entry in the
structure we reuse it, putting the appropriate data in the virtual
structure as described above.  This is fine if we write the exact
same set of variables in subsequent steps, but what if we don't?
Well, if we write a new variable, then the procedure above
happens, but we also take steps to make sure that we generate new
MetaMetaData (I.E. re-register the format with FFS).  We do this
by setting the Info.MetaFormat value to NULL.

Handling a non-written variable is done differently. We don't
really want to bear the cost of new MetaMetaData frequently
(because MetaMetaData can be big), so instead we're willing to
bear the costs of not using some of the data in the virtual
structure.  So if the app Puts an atomic variable on timestep N,
but skips it on N+1, we essentially leave that fraction of the
metadata buffer unused in N+1.  It's transmitted or stored, but it
doesn't contain anything useful.  But the reader still needs to
know that it wasn't written, so BP5 metadata carries with it a
bitmap showing if a variable that is part of the metadata has
actually been written and is valid.  This bitmap, contained in the
BitField[BitFieldCount] fields in the MetadataFieldList is the
ultimate authority as to what has been written.  Variables are
assigned an index in order when they are first entered into
metadata and if the bit at that index isn't set, that variable
wasn't written on that timestep.

Now, this does bring up a vulnerability with BP5.  If an application
were to write a lot of variables on one step and then never use them
again, we might end up with a big metadata block that mostly carried
unused (junk) bytes.  We have not yet run into this in a real
application, so it isn't specifically handled.  In an ideal world, one
would look at the "occcupancy rate" of metadata in EndStep() and make
a decision that for either this timestep or the next, we'd start from
scratch with an empty field list.  There's a tradeoff here.  Do this
too often and we've got big MetaMetadata costs, do it too little and
our metadata has a lot of useless bytes.  Future work.  Note that this
is mostly a writer-side thing to fix/optimize.  The reader will
appropriately handple new metadata, including new metametadata.

The stuff above applies to ADIOS variables, but attributes are always
handled separately.  In the initial FFS-marshalling implementation,
Attributes, while separate, were handled very similarly to variables.
That is, there was a field list and virtual structure maintained where
we entered attributes much like Global and local values are described
above.  There was a metametadata generated it it and it was moved
around like other metametadata blocks.  This old way of doing things
is still present in the code and gets used if `MarshalAttribute()` is
called by the engine.  Engines that use this marshall all attributes
in `Endstep()`, calling MarshalAttribute for all attributes and only
doing this when some attribute has changed.  The resulting Attribute
data always contains ==all== the current attribute values, a situation
that works out well for engines like SST where readers might join
after timestep 0.  The SST writer can save the most recent Attribute
data block and provide it to a newly-joined reader so that it has all
available attributes.

However, this encoding mechanism has some significant disadvantages
under almost all situations.  This separation of metametadata and
metadata was designed for Variables, where the set of variables was
likely to be reused without changes repeatedly.  However, attributes
aren't like that, particularly in the original situation where
attributes once set can never change.  Then we're only doing this when
we add an attribute, we're always generating new MetaMetadata whenever
we have a change, and MetaMetadata + Metadata size is always going to
be bigger than some simpler encoding mechanism.  So, BP5 file engine
now does things differently.  It calls OnetimeMarshalAttribute() which
uses a simpler FFS representation for attributes with the attribute
"name" being part of the data, not part of the metametadata as it is
with variables.  This means that the metametadata never changes, so we
don't have the same issues as with the prior approach.  That
metametadata struct (BP5AttrStruct) describes a relatively simple
structure with two lists, one for attributes of any non-string type,
and the other a list of string and array-of-string attributes.
Generally we only want attributes to appear here when they change, so
the BP5Writer calls OnetimeMarshlAttribute whenever it gets the
NotifyEngineAttribute call (whenever an attribute changes).  However
it also gets called in BeginStep if that step is the first every
called, because some attributes may have been defined before the
engine was ever created.  In BP5 file, attribute blocks then only
every contain an attribute once, unless the attribute changes in which
case it will appear again.  This is not such a good situation for SST
because of the late-coming-reader issue, so that still uses the old
marshaling mechanism.


