#include "png_common.h"

void
png_error_fn(png_structp png_ptr, png_const_charp message)
{
    rb_raise(rb_eRuntimeError, "pnglib: %s", message);
}

void
png_warning_fn(png_structp png_ptr, png_const_charp message)
{
    /* do nothing */
}

void
Init_png()
{
    VALUE mAxon = rb_define_module("Axon");
    rb_const_set(mAxon, rb_intern("PNG_LIB_VERSION"),
		 INT2FIX(PNG_LIBPNG_VER));
}
