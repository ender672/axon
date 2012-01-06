#include <ruby.h>
#include <png.h>

static ID id_write, id_GRAYSCALE, id_RGB, id_gets, id_width, id_height,
	  id_color_model, id_components, id_read;

struct png_data {
    png_structp png_ptr;
    png_infop info_ptr;
    size_t lineno;
    VALUE io;
};

struct io_write {
    VALUE io;
    size_t total;
};

static int
id_to_color_type(ID rb, int components)
{
    if      (rb == id_GRAYSCALE && components == 1) return PNG_COLOR_TYPE_GRAY;
    else if (rb == id_GRAYSCALE && components == 2) return PNG_COLOR_TYPE_GRAY_ALPHA;
    else if (rb == id_RGB       && components == 3) return PNG_COLOR_TYPE_RGB;
    else if (rb == id_RGB       && components == 4) return PNG_COLOR_TYPE_RGB_ALPHA;

    rb_raise(rb_eRuntimeError, "Color Space not recognized.");
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

static void
png_error_fn(png_structp png_ptr, png_const_charp message)
{
    rb_raise(rb_eRuntimeError, "pnglib: %s", message);
}

static void
png_warning_fn(png_structp png_ptr, png_const_charp message)
{
    /* do nothing */
}

void
write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    VALUE str, rb_write_len;
    int write_len;
    struct io_write *iw;

    if (png_ptr == NULL)
	return;

    str = rb_str_new(data, length);
    iw = (struct io_write *)png_get_io_ptr(png_ptr);
    rb_write_len = rb_funcall(iw->io, id_write, 1, str);

    /* Ruby 1.8.7 makes up odd numbers when running NUM2INT on a symbol */
    if (TYPE(rb_write_len) == T_SYMBOL)
	rb_raise(rb_eTypeError, "Number Expected, but got Symbol.");

    write_len = NUM2INT(rb_write_len);

    if ((size_t)write_len != length)
	rb_raise(rb_eRuntimeError, "Write Error. Wrote %d instead of %d bytes.",
		 write_len, (int)length);
    iw->total += length;
}

void
flush_data(png_structp png_ptr)
{
    /* do nothing */
}

static VALUE
write_scanline(VALUE scan_line, png_structp png_ptr, png_infop info_ptr)
{
    png_uint_32 width;
    png_byte components;

    if (TYPE(scan_line) != T_STRING)
	scan_line = rb_obj_as_string(scan_line);

    width = png_get_image_width(png_ptr, info_ptr);
    components = png_get_channels(png_ptr, info_ptr);
    if ((png_uint_32)RSTRING_LEN(scan_line) != width * components)
	rb_raise(rb_eRuntimeError, "Scanline has a bad size. Expected %d * %d but got %d.",
		 (int)width, (int)components, (int)RSTRING_LEN(scan_line));

    png_write_row(png_ptr, (png_bytep)RSTRING_PTR(scan_line));
    return Qnil;
}

static void
write_configure(VALUE image_in, png_structp png_ptr, png_infop info_ptr)
{
    VALUE width, height, color_model, components;
    int color_type;

    width       = rb_funcall(image_in, id_width, 0);
    /* in 1.8.7, NUM2INT gives funny numbers for Symbols */
    if (SYMBOL_P(width))
	rb_raise(rb_eTypeError, "source image has a symbol for width.");

    height      = rb_funcall(image_in, id_height, 0);
    /* in 1.8.7, NUM2INT gives funny numbers for Symbols */
    if (SYMBOL_P(height))
	rb_raise(rb_eTypeError, "source image has a symbol for height.");

    components  = rb_funcall(image_in, id_components, 0);
    /* in 1.8.7, NUM2INT gives funny numbers for Symbols */
    if (SYMBOL_P(components))
	rb_raise(rb_eTypeError, "source image has a symbol for components.");

    color_model = rb_funcall(image_in, id_color_model, 0);
    if (SYMBOL_P(color_model))
	color_type = id_to_color_type(SYM2ID(color_model), NUM2INT(components));
    else
	rb_raise(rb_eTypeError, "source image has a non symbol color space");

    png_set_IHDR(png_ptr, info_ptr, NUM2INT(width), NUM2INT(height), 8,
		 color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);
}

static VALUE
write_png2(VALUE *args)
{
    struct io_write *data;
    png_structp png_ptr = (png_structp)args[0];
    png_infop info_ptr = (png_infop)args[1];
    VALUE scanline, image_in = args[2];
    size_t i;

    write_configure(image_in, png_ptr, info_ptr);
    png_write_info(png_ptr, info_ptr);

    for (i = 0; i < png_get_image_height(png_ptr, info_ptr); i++) {
	scanline = rb_funcall(image_in, id_gets, 0);
	write_scanline(scanline, png_ptr, info_ptr);
    }
    png_write_end(png_ptr, info_ptr);

    data = (struct io_write *)png_get_io_ptr(png_ptr);

    return INT2FIX(data->total);
}

static VALUE
write_png2_ensure(VALUE *args)
{
    png_structp png_ptr = (png_structp)args[0];
    png_infop info_ptr = (png_infop)args[1];

    png_destroy_write_struct(&png_ptr, &info_ptr);
    return Qnil;
}

/*
 *  call-seq:
 *     write(image_in, io_out) -> integer
 *
 *  Writes the given image +image_in+ to +io_out+ as compressed PNG data.
 *  Returns the number of bytes written.
 *
 *     image = Axon::Solid.new(200, 300)
 *     io = File.open("test.jpg", "w")
 *     Axon::PNG.write(image, io)     #=> 1234
 */

static VALUE
write_png(VALUE self, VALUE image_in, VALUE io_out)
{
    VALUE ensure_args[3];
    png_structp png_ptr;
    png_infop info_ptr;
    struct io_write data;

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

    data.io = io_out;
    data.total = 0;
    png_set_write_fn(png_ptr, (void *)&data, write_data, flush_data);

    ensure_args[0] = (VALUE)png_ptr;
    ensure_args[1] = (VALUE)info_ptr;
    ensure_args[2] = image_in;

    return rb_ensure(write_png2, (VALUE)ensure_args, write_png2_ensure,
                     (VALUE)ensure_args);
}

static void
raise_if_locked(struct png_data *reader)
{
    if (reader->lineno)
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
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_info **)NULL);
    else if (png_ptr)
	png_destroy_read_struct(&png_ptr, (png_info **)NULL, (png_info **)NULL);
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

    memcpy(data, RSTRING_PTR(str), read_len);
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
	png_destroy_read_struct(&png_ptr, (png_info **)NULL, (png_info **)NULL);
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

/*
 *  call-seq:
 *     Reader.new(io_in) -> reader
 *
 *  Creates a new PNG Reader. +io_in+ must be an IO-like object that responds
 *  to read(size).
 *
 *     io = File.open("image.png", "r")
 *     reader = Axon::PNG::Reader.new(io)
 */

static VALUE
initialize(VALUE self, VALUE io)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;

    Data_Get_Struct(self, struct png_data, reader);
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    raise_if_locked(reader);
    png_set_read_fn(png_ptr, (void *)io, read_data_fn);
    reader->io = io;
    png_read_info(png_ptr, info_ptr);

    return self;
}

/*
 *  call-seq:
 *     reader.components -> number
 *
 *  Retrieve the number of components as stored in the PNG image.
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
 *  Returns a symbol representing the color model into which the PNG will be
 *  read.
 *
 *  Possible color models are: :GRAYSCALE and :RGB.
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

/*
 *  call-seq:
 *     gets -> string or nil
 *
 *  Reads the next scanline of data from the image.
 *
 *  If the end of the image has been reached, this will return nil.
 */

static VALUE
p_gets(VALUE self)
{
    struct png_data *reader;
    png_structp png_ptr;
    png_infop info_ptr;
    int width, components, sl_width;
    size_t height;
    VALUE sl;

    Data_Get_Struct(self, struct png_data, reader);
    png_ptr = reader->png_ptr;
    info_ptr = reader->info_ptr;

    height = png_get_image_height(png_ptr, info_ptr);
    if (reader->lineno >= height)
	return Qnil;

    width = png_get_image_width(png_ptr, info_ptr);
    components = get_components(png_ptr, info_ptr);
    sl_width = width * components;

    sl = rb_str_new(0, sl_width);
    png_read_row(png_ptr, (png_bytep)RSTRING_PTR(sl), (png_bytep)NULL);
    reader->lineno += 1;

    if (reader->lineno >= height)
	png_read_end(png_ptr, info_ptr);

    return sl;
}

/*
 *  call-seq:
 *     reader.width -> number
 *
 *  Retrieve the width of the image.
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
 *  Retrieve the height of the image.
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

/*
 *  call-seq:
 *     reader.lineno -> number
 *
 *  Returns the number of the next line to be read from the image, starting at
 *  0.
 */

static VALUE
lineno(VALUE self)
{
    struct png_data *reader;
    Data_Get_Struct(self, struct png_data, reader);
    return INT2FIX(reader->lineno);
}

/*
 * Document-class: Axon::PNG::Reader
 *
 * Read compressed PNG images from an IO.
 */

void
Init_PNG()
{
    VALUE mAxon, mPNG, cPNGReader;

    mAxon = rb_define_module("Axon");
    mPNG = rb_define_module_under(mAxon, "PNG");
    rb_const_set(mPNG, rb_intern("LIB_VERSION"), INT2FIX(PNG_LIBPNG_VER));
    rb_define_singleton_method(mPNG, "write", write_png, 2);

    cPNGReader = rb_define_class_under(mPNG, "Reader", rb_cObject);
    rb_define_alloc_func(cPNGReader, allocate);
    rb_define_method(cPNGReader, "initialize", initialize, 1);
    rb_define_method(cPNGReader, "color_model", color_model, 0);
    rb_define_method(cPNGReader, "components", components, 0);
    rb_define_method(cPNGReader, "width", width, 0);
    rb_define_method(cPNGReader, "height", height, 0);
    rb_define_method(cPNGReader, "gets", p_gets, 0);
    rb_define_method(cPNGReader, "lineno", lineno, 0);

    id_GRAYSCALE = rb_intern("GRAYSCALE");
    id_RGB = rb_intern("RGB");
    id_write = rb_intern("write");
    id_read = rb_intern("read");
    id_gets = rb_intern("gets");
    id_width = rb_intern("width");
    id_height = rb_intern("height");
    id_color_model = rb_intern("color_model");
    id_components = rb_intern("components");
}
