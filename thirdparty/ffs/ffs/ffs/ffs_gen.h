typedef struct {
     int address;  /* if TRUE, reg contains the *address* of the data */
     FMdata_type data_type;
     int size;
     int offset;
     int aligned;
     int byte_swap;
     dill_reg vc_reg;
     dill_reg vc_reg2; /* used for paired regs to support non-native data types */
} iogen_oprnd, *iogen_oprnd_ptr;

iogen_oprnd
gen_set(dill_stream c, int size, char*value);

iogen_oprnd
gen_fetch(dill_stream c, dill_reg src_reg, int offset, int size,
		FMdata_type data_type, int aligned, int byte_swap);

iogen_oprnd
gen_operand(dill_reg src_reg, int offset, int size,
		  FMdata_type data_type, int aligned, int byte_swap);

void
gen_load(dill_stream c, iogen_oprnd_ptr src_oprnd);

void
gen_byte_swap(dill_stream c, iogen_oprnd_ptr src_oprnd);

void
gen_store(dill_stream c, 
		iogen_oprnd src, dill_reg dest_reg, int offset, int size,
		FMdata_type data_type, int aligned);

void
gen_memcpy(dill_stream c, dill_reg src, int src_offset, dill_reg dest,
		 int dest_offset, dill_reg size, int const_size);

void
free_oprnd(dill_stream c, iogen_oprnd oprnd);

iogen_oprnd
gen_type_conversion(dill_stream c, iogen_oprnd src_oprnd, FMdata_type data_type);

iogen_oprnd
gen_size_conversion(dill_stream c, iogen_oprnd src_oprnd, int size);

