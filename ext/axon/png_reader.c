#include "png_common.h"

static ID id_read, id_GRAYSCALE, id_RGB;

struct png_data {
    png_structp png_ptr;
    png_infop info_ptr;

    int needs_reset;
    int locked;
    int rewind_after_scanlines;
    
    VALUE io;
};

static void
raise_if_locked(struct png_data *reader)
{
    if (reader->locked)
	rb_raise(rb_eRuntimeError, "Can't modify a locked Reader");
}

static void
free_png(struct png_data *reader)
{
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    if (png_ptr && info_ptr)
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    else if (png_ptr)
	png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
}

static void
free_reader(struct png_data *reader)
{
    free_png(reader);
    free(reader);
}

void
read_data_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    VALUE io, str;
    size_t read_len;

    if (png_ptr == NULL)
	return;

    io = (VALUE)png_get_io_ptr(png_ptr);
    str = rb_funcall(io, id_read, 1, INT2FIX(length));

    if (NIL_P(str))
	rb_raise(rb_eRuntimeError, "Read Error. Reader returned nil.");

    StringValue(str);
    read_len = RSTRING_LEN(str);

    if (read_len != length)
	rb_raise(rb_eRuntimeError, "Read Error. Read %d instead of %d bytes.",
		 (int)read_len, (int)length);

    png_memcpy(data, RSTRING_PTR(str), read_len);
}

static void
mark(struct png_data *reader)
{
    VALUE io = reader->io;

    if (io)
	rb_gc_mark(io);
}

static void
allocate_png(struct png_data *reader)
{
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
				     (png_error_ptr)png_error_fn,
				     (png_error_ptr)png_warning_fn);

    if (png_ptr == NULL)
	rb_raise(rb_eRuntimeError, "unable to allocate a png object");

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
	png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
	rb_raise(rb_eRuntimeError, "unable to allocate a png info object");
    }

    reader->png_ptr = png_ptr;
    reader->info_ptr = info_ptr;
}

static VALUE
allocate(VALUE klass)
{
    VALUE self;
    struct png_data *reader;
    
    self = Data_Make_Struct(klass, struct png_data, mark, free_reader, reader);
    allocate_png(reader);

    return self;
}

VALUE
read_info(struct png_data *reader)
{
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;
    png_read_info(png_ptr, info_ptr);

    return Qnil;
}

/*
 *  call-seq:
 *     Reader.new(string_or_io[, markers]) -> reader
 *
 *  Create a new JPEG Reader. string_or_io may be a String or any object that
 *  responds to read and close.
 *
 *  markers should be an array of valid JPEG header marker symbols. Valid
 *  symbols are :APP0 through :APP15 and :COM.
 *
 *  If performance is important, you can avoid reading all header markers by
 *  supplying an empty array, [].
 *
 *  When markers are not specified, we read all known JPEG markers.
 */
static VALUE
initialize(VALUE self, VALUE io)
{
    struct png_data *reader;

    Data_Get_Struct(self, struct png_data, reader);
    raise_if_locked(reader);
    png_set_read_fn(reader->png_ptr, (void *)io, read_data_fn);
    reader->io = io;
    read_info(reader);

    return self;
}

static int
get_components(png_structp png_ptr, png_infop info_ptr)
{
    switch (png_get_color_type(png_ptr, info_ptr)) {
    case PNG_COLOR_TYPE_GRAY:       return 1;
    case PNG_COLOR_TYPE_GRAY_ALPHA: return 2;
    case PNG_COLOR_TYPE_RGB:        return 3;
    case PNG_COLOR_TYPE_RGB_ALPHA:  return 4;
    }

    return 0;
}

/*
*  call-seq:
*     reader.num_components -> number
*
*  Retrieve the number of components as stored in the JPEG image.
*/
static VALUE
components(VALUE self)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;
    int c;

    Data_Get_Struct(self, struct png_data, reader);
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    c = get_components(png_ptr, info_ptr);

    if (c == 0)
	return Qnil;

    return INT2FIX(c);
}

static ID
png_color_type_to_id(png_byte color_type)
{
    switch (color_type) {
    case PNG_COLOR_TYPE_GRAY:       return id_GRAYSCALE;
    case PNG_COLOR_TYPE_GRAY_ALPHA: return id_GRAYSCALE;
    case PNG_COLOR_TYPE_RGB:        return id_RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA:  return id_RGB;
    }

    rb_raise(rb_eRuntimeError, "PNG Color Type not recognized.");
}

/*
 *  call-seq:
 *     reader.color_model -> symbol
 *
 * Returns a symbol representing the color space into which the JPEG will be
 * transformed as it is read.
 *
 * By default this color space is based on Reader#color_space.
 *
 * Possible color spaces are: GRAYSCALE, RGB, YCbCr, CMYK, and YCCK. This method
 * will return nil if the color space is not recognized.
 */
static VALUE
color_model(VALUE self)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;

    Data_Get_Struct(self, struct png_data, reader);
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    return ID2SYM(png_color_type_to_id(png_get_color_type(png_ptr, info_ptr)));
}

static VALUE
each2(VALUE arg)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;
    int i, width, height, components, sl_width;
    VALUE sl;

    reader = (struct png_data *)arg;
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    components = get_components(png_ptr, info_ptr);
    sl_width = width * components;

    for (i = 0; i < height; i++) {
	sl = rb_str_new(0, sl_width);
	png_read_row(png_ptr, (png_bytep)RSTRING_PTR(sl), png_bytep_NULL);

	if (rb_block_given_p())
	    rb_yield(sl);
    }

    return Qnil;
}

static VALUE
each2_ensure(VALUE arg)
{
    struct png_data *reader;

    reader = (struct png_data *)arg;
    reader->locked = 0;
    reader->needs_reset = 1;

    return Qnil;
}

static void
reset_reader(struct png_data *reader)
{
    VALUE io;

    io = reader->io;
    free_png(reader);
    allocate_png(reader);
    png_set_read_fn(reader->png_ptr, (void *)io, read_data_fn);
    reader->io = io;
    read_info(reader);
}

/*
*  call-seq:
*     reader.each(&block)
*
* Iterate over each decoded scanline in the JPEG image.
*/
static VALUE
each(VALUE self)
{
    struct png_data *reader;

    Data_Get_Struct(self, struct png_data, reader);

    if (reader->needs_reset)
	reset_reader(reader);

    raise_if_locked(reader);
    reader->locked = 1;
    rb_ensure(each2, (VALUE)reader, each2_ensure, (VALUE)reader);

    return Qnil;
}

/*
*  call-seq:
*     reader.width -> number
*
* Retrieve the width of the image as it will be written out. This is primarily
* affected by scale_num and scale_denom if they are set.
* 
* Note that this value is not automatically calculated unless you call
* Reader#calc_output_dimensions or after Reader#each_scanline has been called.
*/
static VALUE
width(VALUE self)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;

    Data_Get_Struct(self, struct png_data, reader);
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    return INT2FIX(png_get_image_width(png_ptr, info_ptr));
}

/*
*  call-seq:
*     reader.height -> number
*
* Retrieve the height of the image as it will be written out. This is primarily
* affected by scale_num and scale_denom if they are set.
* 
* Note that this value is not automatically calculated unless you call
* Reader#calc_output_dimensions or after Reader#each has been called.
*/
static VALUE
height(VALUE self)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;

    Data_Get_Struct(self, struct png_data, reader);
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    return INT2FIX(png_get_image_height(png_ptr, info_ptr));
}

void
Init_png_reader()
{
    VALUE mAxon = rb_define_module("Axon");
    VALUE cPNGReader = rb_define_class_under(mAxon, "PNGReader", rb_cObject);
    VALUE mImage = rb_define_module_under(mAxon, "Image");

    rb_include_module(cPNGReader, mImage);
    rb_include_module(cPNGReader, rb_mEnumerable);

    rb_define_alloc_func(cPNGReader, allocate);

    rb_define_method(cPNGReader, "initialize", initialize, 1);
    rb_define_method(cPNGReader, "color_model", color_model, 0);
    rb_define_method(cPNGReader, "components", components, 0);
    rb_define_method(cPNGReader, "width", width, 0);
    rb_define_method(cPNGReader, "height", height, 0);
    rb_define_method(cPNGReader, "each", each, 0);

    id_read = rb_intern("read");
    id_GRAYSCALE = rb_intern("GRAYSCALE");
    id_RGB = rb_intern("RGB");
}
