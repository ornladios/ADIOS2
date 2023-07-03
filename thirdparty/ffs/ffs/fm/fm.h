#ifndef __FM__
#define __FM__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
#include <stdlib.h>
#include <stdint.h>

typedef struct _FMContextStruct *FMContext;

#if defined(_MSC_VER) && !defined(FFS_SRC)
#define FFS_DECLSPEC    __declspec(dllimport)
#else
#define FFS_DECLSPEC
#endif

extern FFS_DECLSPEC FMContext create_FMcontext();
extern FFS_DECLSPEC FMContext create_local_FMcontext();
extern FFS_DECLSPEC void free_FMcontext(FMContext c);
extern FFS_DECLSPEC void add_ref_FMcontext(FMContext c);
extern FFS_DECLSPEC void FMcontext_allow_self_formats(FMContext fmc);
extern FFS_DECLSPEC int
FMcontext_get_format_server_identifier(FMContext fmc);


#ifndef FMOffset
#define FMOffset(p_type,field) \
	((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#if defined(__STDC__) || defined(__ANSI_CPP__) || defined(_MSC_VER)
#define FMstr(s) #s
#else
#define FMstr(s) "s"
#endif
#define FMArrayDecl(type, size) FMstr(type[size])
#define FMArrayDecl2(type, size, size2) FMstr(type[size][size2])

#define FMDefaultDecl(name, val) FMstr(name(val))

typedef struct _FMField {
    const char *field_name;	/* Field name */
    const char *field_type;	/* Representation type desired */
    int field_size;		/* Size in bytes of representation */
    int field_offset;		/* Offset from base to put field value */
} FMField, *FMFieldList;

typedef struct _FMOptInfo {
    int info_type;
    int info_len;
    const char *info_block;
} FMOptInfo;

/*!
 * A structure to hold Format Name / Field List associations.
 *
 *
 *  This is used to associate names with field lists.  Together these define 
 *  a structure that can be composed into larger structures.
 */
typedef struct _FMformat_list {
    /*! the name to be associated with this structure */
    const char *format_name;
    /*! the FFS-style list of fields within this structure */
    FMFieldList field_list;
    int struct_size;
    FMOptInfo *opt_info;
}FMFormatRec, *FMFormatList, FMStructDescRec, *FMStructDescList;
#endif

typedef enum {
  Format_Unknown= 0, Format_IEEE_754_bigendian = 1, 
  Format_IEEE_754_littleendian = 2, Format_IEEE_754_mixedendian = 3
} FMfloat_format;

typedef enum {
    Format_Integer_Unknown = 0, Format_Integer_bigendian = 1,
    Format_Integer_littleendian = 2
} FMinteger_format;

typedef enum {
    unknown_type, integer_type, unsigned_type, float_type,
    char_type, string_type, enumeration_type, boolean_type
} FMdata_type;

typedef struct _FMFormatBody *FMFormatBodyPtr;
typedef FMFormatBodyPtr FMFormat;

extern FFS_DECLSPEC char *
get_server_rep_FMformat(FMFormat ioformat, int *rep_length);

extern FFS_DECLSPEC char *
get_server_ID_FMformat(FMFormat ioformat, int *id_length);

extern FFS_DECLSPEC FMContext
FMContext_from_FMformat(FMFormat ioformat);

extern FFS_DECLSPEC int
get_rep_len_format_ID(void *format_ID);

extern FFS_DECLSPEC void
set_array_order_FMContext(FMContext iofile, int column_major);

FMFormat
FMformat_from_ID(FMContext iocontext, char *buffer);

int
FMformat_index(FMFormat f);

FMFormat
FMformat_by_index(FMContext c, int index);

extern FFS_DECLSPEC FMFormat
load_external_format_FMcontext(FMContext iocontext, char *server_id,
				     int id_size, char *server_rep);

extern FFS_DECLSPEC void
add_opt_info_FMformat(FMFormat format, int typ, int len, void *block);

extern FFS_DECLSPEC FMFormat
FMregister_simple_format(FMContext context, char *format_name, FMFieldList field_list, int struct_size);

extern FFS_DECLSPEC FMFormat
register_data_format(FMContext context, FMStructDescList struct_list);

extern FFS_DECLSPEC FMFormat
FMregister_data_format(FMContext context, FMStructDescList struct_list);

extern FFS_DECLSPEC void free_FMfield_list(FMFieldList list);

/*!
 * lookup the FMFormat associated with a particular FMStructDescList
 *
 * FMlookup_format() addresses a specific problem particular to libraries.
 * FFSwrite() requires a FMFormat value that results from a
 * register_data_format() call.  Efficiency would dictate that the
 * register_data_format() be performed once and the FMFormat value used
 * repeatedly for multiple writes.  However, libraries which want to avoid
 * the use of static variables, or which wish to support multiple
 * FFS/FMContext values per process have no convenient way to store the
 * FMFormat values for reuse.  CMlookup_format() exploits the fact that
 * field_lists are often constants with fixed addresses (I.E. their memory
 * is not reused for other field lists later in the application).  This call
 * quickly looks up the FMFormat value in an FMcontext by searching for a
 * matching format_list address.
 * 
 * \warning You should *not* use this convenience routine if you cannot
 * guarantee that all field lists used to register formats have a unique
 * address.   Normally if you use static format lists, the addresses will 
 * be unique. 
 */
extern FFS_DECLSPEC FMFormat
FMlookup_format(FMContext context, FMStructDescList struct_list);

typedef enum {Format_Less, Format_Greater, Format_Equal, 
	      Format_Incompatible} FMformat_order;

FMformat_order FMformat_cmp(FMFormat format1, FMFormat format2);

typedef struct compat_formats {
    FMFormat prior_format;
    char *xform_code;
} *FMcompat_formats;

extern FFS_DECLSPEC FMcompat_formats
FMget_compat_formats(FMFormat ioformat);

extern FFS_DECLSPEC char *
name_of_FMformat(FMFormat format);

extern FFS_DECLSPEC FMStructDescList
format_list_of_FMFormat(FMFormat format);

extern FFS_DECLSPEC void
FMlocalize_structs(FMStructDescList list);

extern FFS_DECLSPEC char *
global_name_of_FMFormat(FMFormat format);

extern FFS_DECLSPEC FMFieldList
copy_field_list(FMFieldList list);

extern FFS_DECLSPEC FMStructDescList
FMcopy_struct_list(FMStructDescList list);

extern FFS_DECLSPEC void
FMfree_struct_list(FMStructDescList list);

extern FFS_DECLSPEC int
FMstruct_size_field_list(FMFieldList list, int pointer_size);

extern FFS_DECLSPEC FMStructDescList
FMlocalize_formats(FMStructDescList list);

extern FFS_DECLSPEC int 
count_FMfield(FMFieldList list);

extern FFS_DECLSPEC void print_server_ID(unsigned char *ID);
extern FFS_DECLSPEC void print_format_ID(FMFormat ioformat);
extern FFS_DECLSPEC void fprint_server_ID(void * file,unsigned char *ID);

extern FFS_DECLSPEC int FMformatID_len(char *buffer);

extern FFS_DECLSPEC int
FMdump_data(FMFormat format, void *data, int character_limit);

extern FFS_DECLSPEC int
FMdump_encoded_data(FMFormat format, void *data, int character_limit);

extern FFS_DECLSPEC void
FMdump_XML(FMFormat format, void *data, int encoded);

extern FFS_DECLSPEC void
FMdump_encoded_XML(FMContext c, void *data, int character_limit);

extern FFS_DECLSPEC int
FMfdump_data(void *file, FMFormat format, void *data, int character_limit);

extern FFS_DECLSPEC int
FMfdump_encoded_data(void *file, FMFormat format, void *data, int character_limit);

extern FFS_DECLSPEC void
FMfdump_XML(void *file, FMFormat format, void *data, int encoded);

extern FFS_DECLSPEC void
FMfdump_encoded_XML(void *file, FMContext c, void *data, int character_limit);

extern FFS_DECLSPEC char*
FMunencoded_to_XML_string(FMContext fmcontext, FMFormat format, void *data);

extern FFS_DECLSPEC void
FMfree_var_rec_elements(FMFormat format, void *data);

extern FFS_DECLSPEC char *FMbase_type(const char *field_type);

#define XML_OPT_INFO 0x584D4C20
#define COMPAT_OPT_INFO 0x45564F4C
#define COMPAT_OPT_INFO_FMFILE 0x45564F4D

typedef struct _FMgetFieldStruct *FMFieldPtr;
extern FFS_DECLSPEC FMFieldPtr get_FMfieldPtrFromList(FMFieldList field_list, 
					 const char *fieldname);

extern FFS_DECLSPEC void *
get_FMfieldAddr_by_name(FMFieldList field_list, const char *fieldname, void *data);
extern FFS_DECLSPEC void *
get_FMPtrField_by_name(FMFieldList field_list, const char *fieldname, void *data, int encode);
extern FFS_DECLSPEC int
set_FMPtrField_by_name(FMFieldList field_list, const char *fieldname, void *data, void *ptr_value);
extern FFS_DECLSPEC int
get_FMfieldInt_by_name(FMFieldList field_list, const char *fieldname, void *data);
extern FFS_DECLSPEC size_t
get_FMfieldLong_by_name(FMFieldList field_list, const char *fieldname, void *data);

extern FFS_DECLSPEC void * FMheader_skip(FMContext c, void *data);
extern FFS_DECLSPEC char *get_FMstring_base(FMFieldPtr iofield, void *data, void *string_base);
extern FFS_DECLSPEC void *get_FMFieldAddr(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC void *get_FMaddr (FMFieldPtr iofield, void *data, void *string_base, int encode);
extern FFS_DECLSPEC void *put_FMaddr (FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC float get_FMfloat(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC double get_FMdouble(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC short get_FMshort(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC int get_FMint(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC size_t get_FMlong(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC void get_FMlong8(FMFieldPtr iofield, void *data, unsigned long *low_long, long *high_long);
#if defined(SIZEOF_LONG_LONG)
#if SIZEOF_LONG_LONG != 0
extern FFS_DECLSPEC long long get_FMlong_long(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC unsigned long long get_FMulong_long(FMFieldPtr iofield, void *data);
#endif
#endif
#if defined(SIZEOF_LONG_DOUBLE)
#if SIZEOF_LONG_DOUBLE != 0
extern FFS_DECLSPEC long double get_FMlong_double(FMFieldPtr iofield, void *data);
#endif
#endif
extern FFS_DECLSPEC unsigned short get_FMushort(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC unsigned int get_FMuint(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC size_t get_FMulong(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC int get_FMulong8(FMFieldPtr iofield, void *data, unsigned long *low_long, unsigned long *high_long);
extern FFS_DECLSPEC char *get_FMstring(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC char get_FMchar(FMFieldPtr iofield, void *data);
extern FFS_DECLSPEC int get_FMenum(FMFieldPtr iofield, void *data);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
