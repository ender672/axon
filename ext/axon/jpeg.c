#include <ruby.h>
#include <jpeglib.h>
#include "iccjpeg.h"

/*
 * Marker size is defined by two bytes, so the maximum is 65,535 bytes.
 * Two of those bytes are used by the size indicator bytes themselves, leaving
 * 65,533 bytes.
 */

#define MAX_MARKER_LEN 65533

/*
 * The Exif marker eats 4 bytes for "Exif", and 2 bytes for NULL terminators.
 */

#define EXIF_OVERHEAD_LEN 6
#define MAX_EXIF_DATA_LEN MAX_MARKER_LEN - EXIF_OVERHEAD_LEN
#define EXIF_MARKER (JPEG_APP0 + 1)

#define WRITE_BUFSIZE 1024
#define READ_SIZE 1024

static ID id_ISLOW, id_IFAST, id_FLOAT, id_DEFAULT, id_FASTEST;
static ID id_GRAYSCALE, id_RGB, id_YCbCr, id_CMYK, id_YCCK, id_UNKNOWN;
static ID id_APP0, id_APP1, id_APP2, id_APP3, id_APP4, id_APP5, id_APP6,
	  id_APP7, id_APP8, id_APP9, id_APP10, id_APP11, id_APP12, id_APP13,
	  id_APP14, id_APP15, id_COM;
static ID id_write, id_gets, id_width, id_height, id_color_model, id_read,
	  id_components;
static VALUE sym_icc_profile, sym_exif, sym_quality, sym_bufsize;

static struct jpeg_error_mgr jerr;

struct buf_dest_mgr {
    struct jpeg_destination_mgr pub;
    VALUE io;
    JOCTET *buffer;
    size_t alloc;
    size_t total;
};

struct readerdata {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_source_mgr mgr;

    int header_read;
    int decompress_started;

    VALUE source_io;
    VALUE buffer;
};

static ID
j_dct_method_to_id(J_DCT_METHOD dct_method)
{
    switch (dct_method) {
	case JDCT_ISLOW: return id_ISLOW;
	case JDCT_IFAST: return id_IFAST;
	case JDCT_FLOAT: return id_FLOAT;
    }

    return Qnil;
}

static J_DCT_METHOD
id_to_j_dct_method(ID rb)
{
    if      (rb == id_ISLOW) return JDCT_ISLOW;
    else if (rb == id_IFAST) return JDCT_IFAST;
    else if (rb == id_FLOAT) return JDCT_FLOAT;

    return (J_DCT_METHOD)NULL;
}

static ID
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

static J_COLOR_SPACE
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

static int
sym_to_marker_code(VALUE sym)
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

static void
reset_buffer(struct buf_dest_mgr *dest)
{
    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = dest->alloc;
}

static void
init_destination(j_compress_ptr cinfo)
{
    struct buf_dest_mgr *dest = (struct buf_dest_mgr *) cinfo->dest;
    size_t size = dest->alloc * sizeof(JOCTET);

    /* Allocate the output buffer --- it will be released when done */
    dest->buffer = (JOCTET *) (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
							  JPOOL_IMAGE, size);
    dest->total = 0;
    reset_buffer(dest);
}

static boolean
empty_output_buffer(j_compress_ptr cinfo)
{
    VALUE str, write_len;
    struct buf_dest_mgr *dest = (struct buf_dest_mgr *) cinfo->dest;
    size_t write_len_i;

    str = rb_str_new(dest->buffer, dest->alloc);

    write_len = rb_funcall(dest->io, id_write, 1, str);
    write_len_i = (size_t)NUM2INT(write_len);
    dest->total += write_len_i;
    if (write_len_i != dest->alloc)
	rb_raise(rb_eRuntimeError, "Write Error. Wrote %d instead of %d bytes.",
		 (int)write_len_i, (int)dest->alloc);

    reset_buffer(dest);

    return TRUE;
}

static void
term_destination(j_compress_ptr cinfo)
{
    struct buf_dest_mgr *dest = (struct buf_dest_mgr *) cinfo->dest;
    size_t write_len_i, len = dest->alloc - dest->pub.free_in_buffer;
    VALUE str, write_len;

    if (len > 0) {
	str = rb_str_new(dest->buffer, len);
	write_len = rb_funcall(dest->io, id_write, 1, str);
	write_len_i = (size_t)NUM2INT(write_len);
	dest->total += write_len_i;
	if (write_len_i != len)
	    rb_raise(rb_eRuntimeError, "Write Error. Wrote %d instead of %d bytes.",
		     (int)write_len_i, (int)len);
    }
}

static VALUE
write_scanline(VALUE scan_line, j_compress_ptr cinfo)
{
    JSAMPROW row_pointer[1];

    if (TYPE(scan_line) != T_STRING)
	scan_line = rb_obj_as_string(scan_line);

    if (RSTRING_LEN(scan_line) != cinfo->image_width * cinfo->num_components)
	rb_raise(rb_eRuntimeError, "Scanline has a bad size. Expected %d but got %d.",
		 (int)(cinfo->image_width * cinfo->num_components),
		 (int)RSTRING_LEN(scan_line));

    row_pointer[0] = (JSAMPLE *)RSTRING_PTR(scan_line);
    jpeg_write_scanlines(cinfo, row_pointer, 1);
    return Qnil;
}

static int
write_exif(j_compress_ptr cinfo, char *str, int len)
{
    if (len > MAX_EXIF_DATA_LEN) {
	rb_raise(rb_eRuntimeError, "Exif data is too large. Max is %d.",
		 MAX_EXIF_DATA_LEN);
    }

    jpeg_write_m_header(cinfo, EXIF_MARKER,
			(unsigned int) (len + EXIF_OVERHEAD_LEN));

    jpeg_write_m_byte(cinfo, 0x45);
    jpeg_write_m_byte(cinfo, 0x78);
    jpeg_write_m_byte(cinfo, 0x69);
    jpeg_write_m_byte(cinfo, 0x66);
    jpeg_write_m_byte(cinfo, 0x0);
    jpeg_write_m_byte(cinfo, 0x0);

    /* Add the profile data */
    while (len--) {
	jpeg_write_m_byte(cinfo, *str);
	str++;
    }
}

static void
write_configure(j_compress_ptr cinfo, VALUE image_in, VALUE quality)
{
    VALUE color_model, width, components, rb_height;
    int height;

    width = rb_funcall(image_in, id_width, 0);
    cinfo->image_width = NUM2INT(width);

    rb_height = rb_funcall(image_in, id_height, 0);
    height = NUM2INT(rb_height);
    if (height < 1)
	rb_raise(rb_eRuntimeError, "Source image gave an invalid height.");
    cinfo->image_height = height;

    components = rb_funcall(image_in, id_components, 0);
    cinfo->input_components = NUM2INT(components);

    color_model = rb_funcall(image_in, id_color_model, 0);
    if (SYMBOL_P(color_model))
	cinfo->in_color_space = id_to_j_color_space(SYM2ID(color_model));
    else
	rb_raise(rb_eTypeError, "source image has a non symbol color space");

    jpeg_set_defaults(cinfo);

    if(!NIL_P(quality))
	jpeg_set_quality(cinfo, NUM2INT(quality), TRUE);
}

static void
write_header(j_compress_ptr cinfo, VALUE icc, VALUE exif)
{
    if(!NIL_P(icc)) {
	StringValue(icc);
	write_icc_profile(cinfo, RSTRING_PTR(icc), RSTRING_LEN(icc));
    }

    if (!NIL_P(exif)) {
	StringValue(exif);
	write_exif(cinfo, RSTRING_PTR(exif), RSTRING_LEN(exif));
    }
}

static VALUE
write_jpeg3(VALUE *args)
{
    VALUE image_in, scanline, quality, icc_profile, exif;
    j_compress_ptr cinfo;
    struct buf_dest_mgr *mgr;
    size_t i;

    cinfo = (j_compress_ptr) args[0];
    image_in = args[1];
    quality = args[2];
    icc_profile = args[3];
    exif = args[4];

    write_configure(cinfo, image_in, quality);

    jpeg_start_compress(cinfo, TRUE);

    write_header(cinfo, icc_profile, exif);

    for (i = 0; i < cinfo->image_height; i++) {
	scanline = rb_funcall(image_in, id_gets, 0);
	write_scanline(scanline, cinfo);
    }

    jpeg_finish_compress(cinfo);

    mgr = (struct buf_dest_mgr *)(cinfo->dest);
    return INT2FIX(mgr->total);
}

static VALUE
write_jpeg3_ensure(VALUE *args)
{
    jpeg_destroy_compress((j_compress_ptr) args[0]);
    return INT2FIX(0);
}

static VALUE
write_jpeg2(VALUE image_in, VALUE io_out, size_t bufsize, VALUE icc_profile,
	    VALUE exif, VALUE quality)
{
    struct jpeg_compress_struct cinfo;
    struct buf_dest_mgr mgr;
    VALUE ensure_args[5];

    cinfo.err = &jerr;

    jpeg_create_compress(&cinfo);

    mgr.pub.init_destination = init_destination;
    mgr.pub.empty_output_buffer = empty_output_buffer;
    mgr.pub.term_destination = term_destination;
    mgr.alloc = bufsize;
    mgr.io = io_out;
    cinfo.dest = (struct jpeg_destination_mgr *)&mgr;

    ensure_args[0] = (VALUE)&cinfo;
    ensure_args[1] = image_in;
    ensure_args[2] = quality;
    ensure_args[3] = icc_profile;
    ensure_args[4] = exif;

    return rb_ensure(write_jpeg3, (VALUE)ensure_args, write_jpeg3_ensure,
		     (VALUE)ensure_args);
}

/*
 *  call-seq:
 *     write(image_in, io_out [, options]) -> integer
 *
 *  Writes the given image +image_in+ to +io_out+ as compressed JPEG data.
 *  Returns the number of bytes written.
 *
 *  +options+ may contain the following symbols:
 *
 *     * :bufsize - the size in bytes of the writes that will be made to
 *        +io_out+.
 *     * :quality - the JPEG quality on a 0..100 scale.
 *     * :exif - raw exif data that will be saved in the header.
 *     * :icc_profile - raw icc profile that will be saved in the header.
 *
 *  Example:
 *     image = Axon::Solid.new(200, 300)
 *     io = File.open("test.jpg", "w")
 *     Axon::JPEG.write(image, io)     #=> 1234
 */

static VALUE
write_jpeg(int argc, VALUE *argv, VALUE self)
{
    VALUE image_in, io_out, rb_bufsize, icc_profile, exif, quality, options;
    int bufsize;

    rb_scan_args(argc, argv, "21", &image_in, &io_out, &options);

    bufsize = WRITE_BUFSIZE;

    if (!NIL_P(options) && TYPE(options) == T_HASH) {
	rb_bufsize = rb_hash_aref(options, sym_bufsize);
	if (!NIL_P(rb_bufsize)) {
	    bufsize = NUM2INT(rb_bufsize);
	    if (bufsize < 1)
		rb_raise(rb_eRuntimeError, "Buffer size must be greater than zero");
	}

	icc_profile = rb_hash_aref(options, sym_icc_profile);
	exif = rb_hash_aref(options, sym_exif);
	quality = rb_hash_aref(options, sym_quality);
    } else {
	icc_profile = Qnil;
	exif = Qnil;
	quality = Qnil;
    }

    return write_jpeg2(image_in, io_out, bufsize, icc_profile, exif, quality);
}

static void
error_exit(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    rb_raise(rb_eRuntimeError, "jpeglib: %s", buffer);
}

static void
output_message(j_common_ptr cinfo)
{
    /* do nothing */
}

static void
init_jerror(struct jpeg_error_mgr * err)
{
    jpeg_std_error(err);
    err->error_exit = error_exit;
    err->output_message = output_message;
}

static void
raise_if_locked(struct readerdata *reader)
{
    if (reader->decompress_started)
	rb_raise(rb_eRuntimeError, "Can't modify a Reader after decompress started.");
}

static void
deallocate(struct readerdata *reader)
{
    jpeg_abort_decompress(&reader->cinfo);
    jpeg_destroy_decompress(&reader->cinfo);
    free(reader);
}

static void
mark(struct readerdata *reader)
{
    if (!NIL_P(reader->source_io))
	rb_gc_mark(reader->source_io);

    if (!NIL_P(reader->buffer))
	rb_gc_mark(reader->buffer);
}

/* Data Source Callbacks */

static void
init_source(j_decompress_ptr cinfo)
{
    /* do nothing */
}

static void
term_source(j_decompress_ptr cinfo)
{
    /* do nothing */
}

static void
set_input_buffer(struct readerdata *reader, VALUE string)
{
    size_t nbytes = 0;
    JOCTET *buffer;

    if (!NIL_P(string)) {
	StringValue(string);
	OBJ_FREEZE(string);
	nbytes = (size_t)RSTRING_LEN(string);
	buffer = (JOCTET *)RSTRING_PTR(string);
	reader->buffer = string;
    }

    if (!nbytes) {
	nbytes = 2;
	reader->buffer = rb_str_new(0, 2);

	buffer = (JOCTET *)RSTRING_PTR(reader->buffer);
	buffer[0] = (JOCTET) 0xFF;
	buffer[1] = (JOCTET) JPEG_EOI;
    }

    reader->mgr.next_input_byte = buffer;
    reader->mgr.bytes_in_buffer = nbytes;
}

static boolean
fill_input_buffer(j_decompress_ptr cinfo)
{
    VALUE string;
    struct readerdata *reader;

    reader = (struct readerdata *)cinfo;
    string = rb_funcall(reader->source_io, id_read, 1, INT2FIX(READ_SIZE));
    set_input_buffer(reader, string);

    return TRUE;
}

static void
skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr * src = cinfo->src;

    if (num_bytes > 0) {
	while (num_bytes > (long) src->bytes_in_buffer) {
	    num_bytes -= (long) src->bytes_in_buffer;
	    (void) (*src->fill_input_buffer) (cinfo);
	}
	src->next_input_byte += (size_t) num_bytes;
	src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

static VALUE
allocate(VALUE klass)
{
    struct readerdata *reader;
    VALUE self;

    self = Data_Make_Struct(klass, struct readerdata, mark, deallocate, reader);

    reader->cinfo.err = &jerr;
    jpeg_create_decompress(&reader->cinfo);

    reader->cinfo.src = &reader->mgr;
    reader->mgr.init_source = init_source;
    reader->mgr.fill_input_buffer = fill_input_buffer;
    reader->mgr.skip_input_data = skip_input_data;
    reader->mgr.resync_to_restart = jpeg_resync_to_restart;
    reader->mgr.term_source = term_source;

    return self;
}

static VALUE
read_header2(VALUE arg)
{
    struct readerdata *reader;

    reader = (struct readerdata *)arg;
    jpeg_read_header(&reader->cinfo, TRUE);
    reader->header_read = 1;

    return Qnil;
}

static void
read_header(struct readerdata *reader, VALUE markers)
{
    int i, marker_code, state;
    j_decompress_ptr cinfo;

    cinfo = &reader->cinfo;

    if(NIL_P(markers)) {
	jpeg_save_markers(cinfo, JPEG_COM, 0xFFFF);

	for (i = 0; i < 16; i++)
	    jpeg_save_markers(cinfo, JPEG_APP0 + i, 0xFFFF);
    } else {
	Check_Type(markers, T_ARRAY);
	for (i = 0; i < RARRAY_LEN(markers); i++) {
	    marker_code = sym_to_marker_code(RARRAY_PTR(markers)[i]);
	    jpeg_save_markers(cinfo, marker_code, 0xFFFF);
	}
    }

    rb_protect(read_header2, (VALUE)reader, &state);

    if(state) {
      jpeg_abort_decompress(&reader->cinfo);
      rb_jump_tag(state);
    }

    jpeg_calc_output_dimensions(cinfo);
}

/*
 *  call-seq:
 *     Reader.new(io_in [, markers]) -> reader
 *
 *  Creates a new JPEG Reader. +io_in+ must be an IO-like object that responds
 *  to read(size).
 *
 *  +markers+ should be an array of valid JPEG header marker symbols. Valid
 *  symbols are :APP0 through :APP15 and :COM.
 *
 *  If performance is important, you can avoid reading any header markers by
 *  supplying an empty array, [].
 *
 *  When markers are not specified, we read all known JPEG markers.
 *
 *     io = File.open("image.jpg", "r")
 *     reader = Axon::JPEG::Reader.new(io)
 * 
 *     io = File.open("image.jpg", "r")
 *     reader = Axon::JPEG::Reader.new(io, [:APP4, :APP5])
 */

static VALUE
initialize(int argc, VALUE *argv, VALUE self)
{
    struct readerdata *reader;
    j_decompress_ptr cinfo;
    VALUE io, markers;
    int i;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);
    cinfo = &reader->cinfo;

    rb_scan_args(argc, argv, "11", &io, &markers);

    reader->source_io = io;
    reader->mgr.bytes_in_buffer = 0;

    read_header(reader, markers);

    return self;
}

/*
 *  call-seq:
 *     reader.components -> number
 *
 *  Retrieve the number of components as stored in the JPEG image.
 */

static VALUE
components(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return INT2FIX(cinfo->num_components);
}

/*
 *  call-seq:
 *     reader.in_color_model -> symbol
 *
 *  Returns a symbol representing the color model in which the JPEG is stored.
 *
 *  This does not have to be set explicitly and can be relied upon when the file
 *  conforms to JFIF or Adobe conventions. Otherwise it is guessed.
 *
 *  Possible color models are: :GRAYSCALE, :RGB, :YCbCr, :CMYK, and :YCCK. This
 *  method will return nil if the color model is not recognized.
 */

static VALUE
in_color_model(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    ID id;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    id = j_color_space_to_id(cinfo->jpeg_color_space);

    return ID2SYM(id);
}

/*
 *  call-seq:
 *     reader.in_color_model = symbol
 *
 *  Explicitly sets the color model the JPEG will be read in. This will override
 *  the guessed color model.
 */

static VALUE
set_in_color_model(VALUE self, VALUE cs)
{
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);
    reader->cinfo.jpeg_color_space = id_to_j_color_space(SYM2ID(cs));

    return cs;
}

/*
 *  call-seq:
 *     reader.color_model -> symbol
 *
 *  Returns a symbol representing the color model into which the JPEG will be
 *  transformed as it is read.
 *
 *  Possible color models are: :GRAYSCALE, :RGB, :YCbCr, :CMYK, and :YCCK. This
 *  method will return nil if the color model is not recognized.
 */

static VALUE
color_model(VALUE self)
{
    ID id;
    struct jpeg_decompress_struct * cinfo;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    id = j_color_space_to_id(cinfo->out_color_space);
    return ID2SYM(id);
}

/*
 *  call-seq:
 *     reader.color_model = symbol
 *
 *  Explicitly sets the color model to which the JPEG will be transformed as it
 *  is read.
 *
 *  Legal transformations are:
 *  :YCbCr to :GRAYSCALE, :YCbCr to :RGB, :GRAYSCALE to :RGB, and :YCCK to :CMYK
 */

static VALUE
set_color_model(VALUE self, VALUE cs)
{
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    reader->cinfo.out_color_space = id_to_j_color_space(SYM2ID(cs));
    return cs;
}

/*
 *  call-seq:
 *     reader.scale_num -> number
 *
 *  Retrieve the numerator of the fraction by which the JPEG will be scaled as
 *  it is read. This is always 1 for libjpeg version 6b. In version 8b this can
 *  be 1 to 16.
 */

static VALUE
scale_num(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return INT2FIX(cinfo->scale_num);
}

/*
 *  call-seq:
 *     reader.scale_num = number
 *
 *  Set the numerator of the fraction by which the JPEG will be scaled as it is
 *  read. This must always be 1 for libjpeg version 6b. In version 8b this can
 *  be set to 1 through 16.
 *
 *  Prior to version 1.2, libjpeg-turbo will not scale down images on
 *  decompression, and this option will do nothing.
 */

static VALUE
set_scale_num(VALUE self, VALUE scale_num)
{
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    reader->cinfo.scale_num = NUM2INT(scale_num);
    jpeg_calc_output_dimensions(&reader->cinfo);
    return scale_num;
}

/*
 *  call-seq:
 *     reader.scale_denom -> number
 *
 *  Retrieve the denominator of the fraction by which the JPEG will be scaled as
 *  it is read. This is 1, 2, 4, or 8 for libjpeg version 6b. In version 8b this
 *  is always the source DCT size, which is 8 for baseline JPEG.
 */

static VALUE
scale_denom(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return INT2FIX(cinfo->scale_denom);
}

/*
 *  call-seq:
 *     reader.scale_denom = number
 *
 *  Set the denominator of the fraction by which the JPEG will be scaled as it
 *  is read. This can be set to 1, 2, 4, or 8 for libjpeg version 6b. In version
 *  8b this must always be the source DCT size, which is 8 for baseline JPEG.
 *
 *  Prior to version 1.2, libjpeg-turbo will not scale down images on
 *  decompression, and this option will do nothing.
 */

static VALUE
set_scale_denom(VALUE self, VALUE scale_denom)
{
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    reader->cinfo.scale_denom = NUM2INT(scale_denom);
    jpeg_calc_output_dimensions(&reader->cinfo);
    return scale_denom;
}

/*
 *  call-seq:
 *     reader.dct_method -> symbol
 *
 *  Returns a symbol representing the algorithm used for the DCT (discrete
 *  cosine transform) step in JPEG encoding.
 *
 *  Possible DCT algorithms are: :ISLOW, :IFAST, and :IFLOAT.
 */

static VALUE
dct_method(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    ID id;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    id = j_dct_method_to_id(cinfo->dct_method);

    if (NIL_P(id))
	return Qnil;
    else
	return ID2SYM(id);
}

/*
 *  call-seq:
 *     reader.dct_method = symbol
 *
 *  Sets the algorithm used for the DCT step in JPEG encoding.
 *
 *  Possible DCT algorithms are: :ISLOW, :IFAST, and :IFLOAT.
 */

static VALUE
set_dct_method(VALUE self, VALUE dct_method)
{
    struct readerdata *reader;
    J_DCT_METHOD val;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    val = id_to_j_dct_method(SYM2ID(dct_method));
    if (val == (J_DCT_METHOD)NULL) {
	return Qnil;
    } else {
	reader->cinfo.dct_method = val;
	return dct_method;
    }
}

/*
 *  call-seq:
 *     gets -> string or nil
 *
 *  Reads the next scanline of data from the image. Once the first scanline has
 *  been read you can no longer change read options for this reader.
 *
 *  If the end of the image has been reached, this will return nil.
 */

static VALUE
j_gets(VALUE self)
{
    struct readerdata *reader;
    struct jpeg_decompress_struct *cinfo;
    VALUE sl;
    int sl_width, ret;
    JSAMPROW ijg_buffer;

    Data_Get_Struct(self, struct readerdata, reader);
    cinfo = &reader->cinfo;

    if (!reader->header_read)
      read_header(reader, Qnil);

    if (!reader->decompress_started) {
	reader->decompress_started = 1;
	jpeg_start_decompress(cinfo);
    }

    sl_width = cinfo->output_width * cinfo->output_components;
    sl = rb_str_new(0, sl_width);
    ijg_buffer = (JSAMPROW)RSTRING_PTR(sl);

    ret = jpeg_read_scanlines(cinfo, &ijg_buffer, 1);
    return ret == 0 ? Qnil : sl;
}

/*
 *  call-seq:
 *     reader.width -> number
 *
 *  Retrieve the width of the image as it will be written out. This can be
 *  affected by scale_num and scale_denom if they are set.
 */

static VALUE
width(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return INT2FIX(cinfo->output_width);
}

/*
 *  call-seq:
 *     reader.height -> number
 *
 *  Retrieve the height of the image as it will be written out. This can be
 *  affected by scale_num and scale_denom if they are set.
 */

static VALUE
height(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return INT2FIX(cinfo->output_height);
}

/*
 *  call-seq:
 *     reader.icc_profile -> string
 *
 *  Read the raw icc_profile from the JPEG. This requires that the APP2 segment
 *  has been selected by initialize (this is the default behaviour).
 */

static VALUE
icc_profile(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    JOCTET *icc_embed_buffer;
    unsigned int icc_embed_len;
    VALUE str;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    read_icc_profile(cinfo, &icc_embed_buffer, &icc_embed_len);

    if (icc_embed_len <= 0) {
	return Qnil;
    } else {
	str = rb_str_new(icc_embed_buffer, icc_embed_len);
	free(icc_embed_buffer);
	return str;
    }
}

static boolean
marker_is_exif(jpeg_saved_marker_ptr marker)
{
    return marker->marker == EXIF_MARKER &&
	   marker->data_length >= EXIF_OVERHEAD_LEN &&
	   /* verify the identifying string */
	   GETJOCTET(marker->data[0]) == 0x45 &&
	   GETJOCTET(marker->data[1]) == 0x78 &&
	   GETJOCTET(marker->data[2]) == 0x69 &&
	   GETJOCTET(marker->data[3]) == 0x66 &&
	   GETJOCTET(marker->data[4]) == 0x0  &&
	   GETJOCTET(marker->data[5]) == 0x0;
}

/*
 *  call-seq:
 *     reader.exif -> string
 *
 *  Get the raw Exif data from the JPEG. This requires that the APP1 segment
 *  has been selected by initialize (this is the default behavior).
 */

static VALUE
exif(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    jpeg_saved_marker_ptr marker;
    int len;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
	if (marker_is_exif(marker)) {
	    len = marker->data_length - EXIF_OVERHEAD_LEN;
	    return rb_str_new(marker->data + EXIF_OVERHEAD_LEN, len);
	}
    }

    return Qnil;
}

/*
 *  call-seq:
 *     reader[marker] -> array
 *
 *  Read raw data from the given JPEG header marker. Note that the marker must
 *  have been specified by initialize.
 *
 *  The return from this method is an array, since there may be multiple
 *  instances of a single marker in a JPEG header.
 */

static VALUE
aref(VALUE self, VALUE marker_sym)
{
    struct jpeg_decompress_struct * cinfo;
    jpeg_saved_marker_ptr marker;
    VALUE ary = rb_ary_new();
    int marker_i = sym_to_marker_code(marker_sym);

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    for (marker = cinfo->marker_list; marker != NULL; marker = marker->next)
	if (marker->marker == marker_i)
	    rb_ary_push(ary, rb_str_new(marker->data, marker->data_length));

    return ary;
}

/*
 *  call-seq:
 *     reader.saw_jfif_marker -> boolean
 *
 *  Indicates whether a JFIF marker was found in the header.
 */

static VALUE
saw_jfif_marker(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return cinfo->saw_JFIF_marker ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *     reader.saw_adobe_marker -> boolean
 *
 *  Indicates whether an Adobe marker was found in the header.
 */

static VALUE
saw_adobe_marker(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return cinfo->saw_Adobe_marker ? Qtrue : Qfalse;
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
    struct jpeg_decompress_struct * cinfo;
    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);
    return INT2FIX(cinfo->output_scanline);
}

/*
 * Document-class: Axon::JPEG::Reader
 *
 * Read compressed JPEG images from an IO.
 */

void
Init_JPEG()
{
    VALUE mAxon, mJPEG, cJPEGReader;

    init_jerror(&jerr);

    mAxon = rb_define_module("Axon");
    mJPEG = rb_define_module_under(mAxon, "JPEG");
    rb_const_set(mJPEG, rb_intern("LIB_VERSION"), INT2FIX(JPEG_LIB_VERSION));
#ifdef JCS_EXTENSIONS
    rb_const_set(mJPEG, rb_intern("LIB_TURBO"), Qtrue);
#else
    rb_const_set(mJPEG, rb_intern("LIB_TURBO"), Qfalse);
#endif
    rb_define_singleton_method(mJPEG, "write", write_jpeg, -1);

    cJPEGReader = rb_define_class_under(mJPEG, "Reader", rb_cObject);
    rb_define_alloc_func(cJPEGReader, allocate);
    rb_define_method(cJPEGReader, "initialize", initialize, -1);
    rb_define_method(cJPEGReader, "icc_profile", icc_profile, 0);
    rb_define_method(cJPEGReader, "exif", exif, 0);
    rb_define_method(cJPEGReader, "saw_jfif_marker", saw_jfif_marker, 0);
    rb_define_method(cJPEGReader, "saw_adobe_marker", saw_adobe_marker, 0);
    rb_define_method(cJPEGReader, "[]", aref, 1);
    rb_define_method(cJPEGReader, "in_color_model", in_color_model, 0);
    rb_define_method(cJPEGReader, "in_color_model=", set_in_color_model, 1);
    rb_define_method(cJPEGReader, "color_model", color_model, 0);
    rb_define_method(cJPEGReader, "color_model=", set_color_model, 1);
    rb_define_method(cJPEGReader, "components", components, 0);
    rb_define_method(cJPEGReader, "scale_num", scale_num, 0);
    rb_define_method(cJPEGReader, "scale_num=", set_scale_num, 1);
    rb_define_method(cJPEGReader, "scale_denom", scale_denom, 0);
    rb_define_method(cJPEGReader, "scale_denom=", set_scale_denom, 1);
    rb_define_method(cJPEGReader, "dct_method", dct_method, 0);
    rb_define_method(cJPEGReader, "dct_method=", set_dct_method, 1);
    rb_define_method(cJPEGReader, "width", width, 0);
    rb_define_method(cJPEGReader, "height", height, 0);
    rb_define_method(cJPEGReader, "lineno", lineno, 0);
    rb_define_method(cJPEGReader, "gets", j_gets, 0);

    id_IFAST = rb_intern("IFAST");
    id_ISLOW = rb_intern("ISLOW");
    id_FLOAT = rb_intern("FLOAT");
    id_DEFAULT = rb_intern("DEFAULT");
    id_FASTEST = rb_intern("FASTEST");

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

    id_read = rb_intern("read");
    id_write = rb_intern("write");
    id_width = rb_intern("width");
    id_height = rb_intern("height");
    id_color_model = rb_intern("color_model");
    id_components = rb_intern("components");
    id_gets = rb_intern("gets");

    sym_icc_profile = ID2SYM(rb_intern("icc_profile"));
    sym_exif = ID2SYM(rb_intern("exif"));
    sym_quality = ID2SYM(rb_intern("quality"));
    sym_bufsize = ID2SYM(rb_intern("bufsize"));

    rb_const_set(cJPEGReader, rb_intern("DEFAULT_DCT"),
		 ID2SYM(j_dct_method_to_id(JDCT_DEFAULT)));
    rb_const_set(cJPEGReader, rb_intern("FASTEST_DCT"),
		 ID2SYM(j_dct_method_to_id(JDCT_FASTEST)));
}
