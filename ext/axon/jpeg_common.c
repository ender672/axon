#include "jpeg_common.h"

static ID id_GRAYSCALE, id_RGB, id_YCbCr, id_CMYK, id_YCCK, id_UNKNOWN;
static ID id_APP0, id_APP1, id_APP2, id_APP3, id_APP4, id_APP5, id_APP6,
	  id_APP7, id_APP8, id_APP9, id_APP10, id_APP11, id_APP12, id_APP13,
	  id_APP14, id_APP15, id_COM;

struct jpeg_error_mgr jerr;

ID
j_color_space_to_id(J_COLOR_SPACE cs)
{
    switch (cs) {
	case JCS_GRAYSCALE: return id_GRAYSCALE;
	case JCS_RGB:       return id_RGB;
	case JCS_YCbCr:     return id_YCbCr;
	case JCS_CMYK:      return id_CMYK;
	case JCS_YCCK:      return id_YCCK;
    }

    return id_UNKNOWN;
}

J_COLOR_SPACE
id_to_j_color_space(ID rb)
{
    if      (rb == id_GRAYSCALE) return JCS_GRAYSCALE;
    else if (rb == id_RGB)       return JCS_RGB;
    else if (rb == id_YCbCr)     return JCS_YCbCr;
    else if (rb == id_CMYK)      return JCS_CMYK;
    else if (rb == id_YCCK)      return JCS_YCCK;
    else if (rb == id_UNKNOWN)   return JCS_UNKNOWN;

    rb_raise(rb_eRuntimeError, "Color Space not recognized.");
}

int sym_to_marker_code(VALUE sym)
{
    ID rb = SYM2ID(sym);

    if      (rb == id_APP0)  return JPEG_APP0;
    else if (rb == id_APP1)  return JPEG_APP0 + 1;
    else if (rb == id_APP2)  return JPEG_APP0 + 2;
    else if (rb == id_APP3)  return JPEG_APP0 + 3;
    else if (rb == id_APP4)  return JPEG_APP0 + 4;
    else if (rb == id_APP5)  return JPEG_APP0 + 5;
    else if (rb == id_APP6)  return JPEG_APP0 + 6;
    else if (rb == id_APP7)  return JPEG_APP0 + 7;
    else if (rb == id_APP8)  return JPEG_APP0 + 8;
    else if (rb == id_APP9)  return JPEG_APP0 + 9;
    else if (rb == id_APP10) return JPEG_APP0 + 10;
    else if (rb == id_APP11) return JPEG_APP0 + 11;
    else if (rb == id_APP12) return JPEG_APP0 + 12;
    else if (rb == id_APP13) return JPEG_APP0 + 13;
    else if (rb == id_APP14) return JPEG_APP0 + 14;
    else if (rb == id_APP15) return JPEG_APP0 + 15;
    else if (rb == id_COM)   return JPEG_COM;

    rb_raise(rb_eRuntimeError, "Marker code not recognized.");
}

void
error_exit(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    (*cinfo->err->format_message) (cinfo, buffer);
    rb_raise(rb_eRuntimeError, "jpeglib: %s", buffer);
}

void
output_message(j_common_ptr cinfo)
{
    /* do nothing */
}

void
init_jerror(struct jpeg_error_mgr * err)
{
    jpeg_std_error(err);
    err->error_exit = error_exit;
    err->output_message = output_message;
}

void
Init_jpeg()
{
    VALUE mAxon = rb_define_module("Axon");
    rb_const_set(mAxon, rb_intern("JPEG_LIB_VERSION"),
		 INT2FIX(JPEG_LIB_VERSION));

#ifdef JCS_EXTENSIONS
    rb_const_set(mAxon, rb_intern("JPEG_LIB_TURBO"), Qtrue);
#else
    rb_const_set(mAxon, rb_intern("JPEG_LIB_TURBO"), Qfalse);
#endif
    
    init_jerror(&jerr);

    id_GRAYSCALE = rb_intern("GRAYSCALE");
    id_RGB = rb_intern("RGB");
    id_YCbCr = rb_intern("YCbCr");
    id_CMYK = rb_intern("CMYK");
    id_YCCK = rb_intern("YCCK");
    id_UNKNOWN = rb_intern("UNKNOWN");

    id_APP0 = rb_intern("APP0");
    id_APP1 = rb_intern("APP1");
    id_APP2 = rb_intern("APP2");
    id_APP3 = rb_intern("APP3");
    id_APP4 = rb_intern("APP4");
    id_APP5 = rb_intern("APP5");
    id_APP6 = rb_intern("APP6");
    id_APP7 = rb_intern("APP7");
    id_APP8 = rb_intern("APP8");
    id_APP9 = rb_intern("APP9");
    id_APP10 = rb_intern("APP10");
    id_APP11 = rb_intern("APP11");
    id_APP12 = rb_intern("APP12");
    id_APP13 = rb_intern("APP13");
    id_APP14 = rb_intern("APP14");
    id_APP15 = rb_intern("APP15");
    id_COM = rb_intern("COM");
}
