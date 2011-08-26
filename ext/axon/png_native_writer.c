#include "png_common.h"

static ID id_write, id_GRAYSCALE, id_RGB, id_image, id_each, id_width,
	  id_height, id_color_model, id_components;

void
write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    VALUE str, io, write_len;
    int write_len_i;

    if (png_ptr == NULL)
	return;

    str = rb_str_new(data, length);
    io = (VALUE)png_get_io_ptr(png_ptr);
    write_len = rb_funcall(io, id_write, 1, str);
    write_len_i = FIX2INT(write_len);
    
    if ((size_t)write_len_i != length)
	rb_raise(rb_eRuntimeError, "Write Error. Wrote %d instead of %d bytes.",
		 write_len_i, (int)length);
}

void
flush_data(png_structp png_ptr)
{
    /* do nothing */
}

static VALUE
write_scanline(VALUE scan_line, VALUE *args)
{
    png_structp png_ptr = (png_structp)args[1];
    png_infop info_ptr = (png_infop)args[2];
    png_uint_32 width;
    png_byte components;

    if (TYPE(scan_line) != T_STRING)
	scan_line = rb_obj_as_string(scan_line);

    width = png_get_image_width(png_ptr, info_ptr);
    components = png_get_channels(png_ptr, info_ptr);
    if ((png_uint_32)RSTRING_LEN(scan_line) != width * components)
	rb_raise(rb_eRuntimeError, "Scanline has a bad size. Expected %d but got %d.",
		 (int)(width * components), (int)RSTRING_LEN(scan_line));

    png_write_row(png_ptr, (png_bytep)RSTRING_PTR(scan_line));
    return Qnil;
}

static int
id_to_color_type(ID rb, int components)
{
    if      (rb == id_GRAYSCALE && components == 1) return PNG_COLOR_TYPE_GRAY;
    else if (rb == id_GRAYSCALE && components == 2) return PNG_COLOR_TYPE_GRAY_ALPHA;
    else if (rb == id_RGB       && components == 3) return PNG_COLOR_TYPE_RGB;
    else if (rb == id_RGB       && components == 4) return PNG_COLOR_TYPE_RGB_ALPHA;

    rb_raise(rb_eRuntimeError, "Color Space not recognized.");
}

static void
size_and_colors(VALUE self, png_structp png_ptr, png_infop info_ptr)
{
    VALUE image, width, height, color_model, components;
    int color_type;

    image = rb_funcall(self, id_image, 0);

    width       = rb_funcall(image, id_width, 0);
    height      = rb_funcall(image, id_height, 0);
    components  = rb_funcall(image, id_components, 0);
    color_model = rb_funcall(image, id_color_model, 0);

    color_type = id_to_color_type(SYM2ID(color_model), FIX2INT(components));

    png_set_IHDR(png_ptr, info_ptr, FIX2INT(width), FIX2INT(height), 8,
		 color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);
}

static VALUE
write2(VALUE *args)
{
    VALUE self = args[0];
    png_structp png_ptr = (png_structp)args[1];
    png_infop info_ptr = (png_infop)args[2];

    VALUE image = rb_funcall(self, id_image, 0);

    size_and_colors(self, png_ptr, info_ptr);

    png_write_info(png_ptr, info_ptr);
    rb_block_call(image, id_each, 0, 0, write_scanline, (VALUE)args);
    png_write_end(png_ptr, info_ptr);

    return Qnil;
}

static VALUE
write2_ensure(VALUE *args)
{
    png_structp png_ptr = (png_structp)args[1];
    png_infop info_ptr = (png_infop)args[2];

    png_destroy_write_struct(&png_ptr, &info_ptr);
    return Qnil;
}

/*
*  call-seq:
*     writer.write(io) -> nil
*
*  Compress image and write the png to io.
*/

static VALUE
pngwrite(VALUE self, VALUE io)
{
    VALUE ensure_args[3];

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
				      (png_error_ptr)png_error_fn,
				      (png_error_ptr)png_warning_fn);

    if (png_ptr == NULL)
	rb_raise(rb_eRuntimeError, "unable to allocate a png object");

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
	png_destroy_write_struct(&png_ptr, (png_info **)NULL);
	rb_raise(rb_eRuntimeError, "unable to allocate a png info object");
    }

    png_set_write_fn(png_ptr, (void *)io, write_data, flush_data);

    ensure_args[0] = self;
    ensure_args[1] = (VALUE)png_ptr;
    ensure_args[2] = (VALUE)info_ptr;

    return rb_ensure(write2, (VALUE)ensure_args, write2_ensure,
                     (VALUE)ensure_args);
}

void
Init_png_native_writer()
{
    VALUE mAxon = rb_define_module("Axon");
    VALUE mNativeWriter = rb_define_module_under(mAxon, "PNGNativeWriter");

    rb_define_method(mNativeWriter, "png_native_write", pngwrite, 1);

    id_GRAYSCALE = rb_intern("GRAYSCALE");
    id_RGB = rb_intern("RGB");
    id_write = rb_intern("write");
    id_image = rb_intern("image");
    id_each = rb_intern("each");
    id_width = rb_intern("width");
    id_height = rb_intern("height");
    id_color_model = rb_intern("color_model");
    id_components = rb_intern("components");
}
