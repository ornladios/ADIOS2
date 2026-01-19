#include <string.h>
#include <stdio.h>

typedef enum {
  Format_Unknown= 0, Format_IEEE_754_bigendian = 1, 
  Format_IEEE_754_littleendian = 2, Format_IEEE_754_mixedendian = 3
} FMfloat_format;

static unsigned char IEEE_754_4_bigendian[] = 
  {0x3c, 0x00, 0x00, 0x00};
static unsigned char IEEE_754_4_littleendian[] = 
  {0x00, 0x00, 0x00, 0x3c};
static unsigned char IEEE_754_8_bigendian[] = 
  {0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char IEEE_754_8_littleendian[] = 
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f};
static unsigned char IEEE_754_8_mixedendian[] = 
  {0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00};
static unsigned char IEEE_754_16_bigendian[] = 
  {0x3f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char IEEE_754_16_littleendian[] = 
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x3f};
static unsigned char IEEE_754_16_mixedendian[] = 
  {0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static FMfloat_format
infer_float_format(char *float_magic, int object_len)
{
    switch (object_len) {
    case 4:
	if (memcmp(float_magic, &IEEE_754_4_bigendian[0], 4) == 0) {
	    return Format_IEEE_754_bigendian;
	} else if (memcmp(float_magic, &IEEE_754_4_littleendian[0], 4) == 0) {
	    return Format_IEEE_754_littleendian;
	}
	break;
    case 8:
	if (memcmp(float_magic, &IEEE_754_8_bigendian[0], 8) == 0) {
	    return Format_IEEE_754_bigendian;
	} else if (memcmp(float_magic, &IEEE_754_8_littleendian[0], 8) == 0) {
	    return Format_IEEE_754_littleendian;
	} else if (memcmp(float_magic, &IEEE_754_8_mixedendian[0], 8) == 0) {
	    return Format_IEEE_754_mixedendian;
	}
	break;
    case 16:
	if (memcmp(float_magic, &IEEE_754_16_bigendian[0], 16) == 0) {
	    return Format_IEEE_754_bigendian;
	} else if (memcmp(float_magic, &IEEE_754_16_littleendian[0], 16) ==0){
	    return Format_IEEE_754_littleendian;
	} else if (memcmp(float_magic, &IEEE_754_16_mixedendian[0], 16) == 0){
	    return Format_IEEE_754_mixedendian;
	}
	break;
    }
    return Format_Unknown;
}

#define MAGIC_FLOAT 0.0078125	/* random float */

int
main()
{
    // the textual output of this must be kept in sync with the enumeration in fm/fm.h 
    double d = MAGIC_FLOAT;
    FMfloat_format cur_float_format = infer_float_format((char*)&d, sizeof(d));
    switch (cur_float_format) {
    case Format_IEEE_754_bigendian:
        printf("Format_IEEE_754_bigendian");
        break;
    case Format_IEEE_754_littleendian:
        printf("Format_IEEE_754_littleendian");
        break;
    case Format_IEEE_754_mixedendian:
        printf("Format_IEEE_754_mixedendian");
        break;
    case Format_Unknown:
        fprintf(stderr, "Format_Unknown");
        break;
    }
}
