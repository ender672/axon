#ifndef  AXON_PNG_COMMON_H
#define  AXON_PNG_COMMON_H

#include <ruby.h>
#include <png.h>

#ifndef HAVE_RB_BLOCK_CALL
#define rb_block_call(arg1, arg2, arg3, arg4, arg5, arg6) rb_iterate(rb_each, arg1, arg5, arg6)
#endif

void png_error_fn(png_structp, png_const_charp);
void png_warning_fn(png_structp, png_const_charp);

void Init_png();
void Init_png_native_writer();
void Init_png_reader();

#endif
