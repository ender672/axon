#include "jpeg_common.h"

static ID id_read, id_rewind, id_ISLOW, id_IFAST, id_FLOAT, id_DEFAULT,
	  id_FASTEST;

struct readerdata {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_source_mgr mgr;

    int header_read;
    int locked;
    int rewind_after_scanlines;

    VALUE source_io;
    VALUE buffer;
};

static void
raise_if_locked(struct readerdata *reader)
{
    if (reader->locked)
	rb_raise(rb_eRuntimeError, "Can't modify a locked Reader");
}

static void
deallocate(struct readerdata *reader)
{
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
    string = rb_funcall(reader->source_io, id_read, 0);
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

    if (!NIL_P(markers)) {
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
 *     Reader.new(io[, markers, rewind_after_scanlines]) -> reader
 *
 *  Create a new JPEG Reader. string_or_io may be an object that responds to
 *  read or a string.
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
initialize(int argc, VALUE *argv, VALUE self)
{
    struct readerdata *reader;
    j_decompress_ptr cinfo;
    VALUE io, markers, rewind_after_scanlines;
    int i;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);
    cinfo = &reader->cinfo;

    rb_scan_args(argc, argv, "12", &io, &markers, &rewind_after_scanlines);

    reader->source_io = io;
    reader->mgr.bytes_in_buffer = 0;

    if(NIL_P(markers)) {
	jpeg_save_markers(cinfo, JPEG_COM, 0xFFFF);

	for (i = 0; i < 16; i++)
	    jpeg_save_markers(cinfo, JPEG_APP0 + i, 0xFFFF);
    }

    reader->rewind_after_scanlines = RTEST(rewind_after_scanlines);
    read_header(reader, markers);

    return self;
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
    struct jpeg_decompress_struct * cinfo;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    return INT2FIX(cinfo->num_components);
}

/*
*  call-seq:
*     reader.in_color_model -> symbol
*
* Returns a symbol representing the color space in which the JPEG is stored.
* 
* This does not have to be set explicitly and can be relied upon when the file
* conforms to JFIF or Adobe conventions. Otherwise it is guessed.
* 
* Possible color spaces are: GRAYSCALE, RGB, YCbCr, CMYK, and YCCK. This method
* will return nil if the color space is not recognized.
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
 *     reader.color_space = symbol
 *
 * Explicitly sets the color space the JPEG will be read in. This will override
 * the guessed color space.
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
 * Explicitly sets the color space to which the JPEG will be transformed as it
 * is read.
 *
 * Legal transformations are:
 * YCbCr to GRAYSCALE, YCbCr to RGB, GRAYSCALE to RGB, and YCCK to CMYK
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
* Retrieve the numerator of the fraction by which the JPEG will be scaled as
* it is read. This is always 1 for libjpeg version 6b. In version 8b this can
* be 1 to 16.
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
* Set the numerator of the fraction by which the JPEG will be scaled as it is
* read. This must always be 1 for libjpeg version 6b. In version 8b this can
* be set to 1 through 16.
*/
static VALUE
set_scale_num(VALUE self, VALUE scale_num)
{
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    reader->cinfo.scale_num = FIX2INT(scale_num);
    jpeg_calc_output_dimensions(&reader->cinfo);
    return scale_num;
}

/*
*  call-seq:
*     reader.scale_denom -> number
*
* Retrieve the denominator of the fraction by which the JPEG will be scaled as
* it is read. This is 1, 2, 4, or 8 for libjpeg version 6b. In version 8b this
* is always the source DCT size, which is 8 for baseline JPEG.
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
* Set the denominator of the fraction by which the JPEG will be scaled as it is
* read. This can be set to 1, 2, 4, or 8 for libjpeg version 6b. In version 8b
* this must always be the source DCT size, which is 8 for baseline JPEG.
*/
static VALUE
set_scale_denom(VALUE self, VALUE scale_denom)
{
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    reader->cinfo.scale_denom = FIX2INT(scale_denom);
    jpeg_calc_output_dimensions(&reader->cinfo);
    return scale_denom;
}

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

/*
*  call-seq:
*     reader.dct_method -> symbol
*
* Returns a symbol representing the algorithm used for the DCT (discrete cosine
* transform) step in JPEG encoding.
* 
* Possible DCT algorithms are: ISLOW, IFAST, and IFLOAT.
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
* Sets the algorithm used for the DCT step in JPEG encoding.
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

static VALUE
each3(VALUE arg)
{
    struct jpeg_decompress_struct * cinfo;
    struct readerdata *reader;
    VALUE sl;
    int width, height, components, sl_width, i;
    JSAMPROW ijg_buffer;

    reader = (struct readerdata *)arg;
    cinfo = &reader->cinfo;

    width      = cinfo->output_width;
    height     = cinfo->output_height;
    components = cinfo->output_components;

    sl_width = width * components;

    for (i = 0; i < height; i++) {
	sl = rb_str_new(0, sl_width);
	ijg_buffer = (JSAMPROW)RSTRING_PTR(sl);
	jpeg_read_scanlines(cinfo, &ijg_buffer, 1);

	if (rb_block_given_p())
	    rb_yield(sl);
    }

    jpeg_finish_decompress(cinfo);

    return Qnil;
}

static VALUE
each3_rescue(VALUE arg)
{
    struct readerdata *reader;

    reader = (struct readerdata *)arg;
    jpeg_abort_decompress(&reader->cinfo);

    return Qnil;
}

static VALUE
each2(VALUE arg)
{
    rb_rescue(each3, arg, each3_rescue, arg);
}

static VALUE
each2_ensure(VALUE arg)
{
    struct readerdata *reader;

    reader = (struct readerdata *)arg;
    reader->locked = 0;
    reader->header_read = 0;
    if (reader->rewind_after_scanlines)
	rb_funcall(reader->source_io, id_rewind, 0);

    return Qnil;
}

/*
 *  call-seq:
 *     reader.each_scanline(&block)
 *
 * Iterate over each decoded scanline in the JPEG image. During this operation
 * the reader is locked, and you can't change any decoding parameters or re
 * initialize the reader.
 *
 * Should a major exception occur (anything other than a StandardError), then
 * the reader will be left in a locked state.
 */
static VALUE
each(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;
    struct readerdata *reader;

    Data_Get_Struct(self, struct readerdata, reader);
    raise_if_locked(reader);

    if (!reader->header_read)
      read_header(reader, Qnil);

    reader->locked = 1;
    jpeg_start_decompress(&reader->cinfo);

    rb_ensure(each2, (VALUE)reader, each2_ensure, (VALUE)reader);

    return self;
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
    struct jpeg_decompress_struct * cinfo;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    return INT2FIX(cinfo->output_width);
}

/*
*  call-seq:
*     reader.height -> number
*
* Retrieve the height of the image as it will be written out. This is primarily
* affected by scale_num and scale_denom if they are set.
* 
* Note that this value is not automatically calculated unless you call
* Reader#calc_output_dimensions or after Reader#each_scanline has been called.
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
* Read the icc_profile from the JPEG. This requires that the APP2 segment
* has been selected by save_markers (this is the default behaviour).
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
* Read the Exif data from the JPEG. This requires that the APP1 segment
* has been selected by save_markers (this is the default behaviour).
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
 * Read raw data from the given JPEG header marker. Note that the marker must
 * have been specified by Reader#save_markers prior to the call to
 * Reader#read_header.
 *
 * The return from this method is an array, since there may be multiple
 * instances of a single marker in a JPEG header.
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
*  Indicate that a JFIF marker was found in the header.
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
*  Indicate that an Adobe marker was found in the header.
*/
static VALUE
saw_adobe_marker(VALUE self)
{
    struct jpeg_decompress_struct * cinfo;

    Data_Get_Struct(self, struct jpeg_decompress_struct, cinfo);

    return cinfo->saw_Adobe_marker ? Qtrue : Qfalse;
}

void
Init_jpeg_reader()
{
    VALUE mAxon = rb_define_module("Axon");
    VALUE cJPEGReader = rb_define_class_under(mAxon, "JPEGReader", rb_cObject);
    VALUE mImage = rb_define_module_under(mAxon, "Image");

    rb_include_module(cJPEGReader, mImage);
    rb_include_module(cJPEGReader, rb_mEnumerable);

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
    rb_define_method(cJPEGReader, "each", each, 0);

    id_read = rb_intern("read");
    id_rewind = rb_intern("rewind");
    id_IFAST = rb_intern("IFAST");
    id_ISLOW = rb_intern("ISLOW");
    id_FLOAT = rb_intern("FLOAT");
    id_DEFAULT = rb_intern("DEFAULT");
    id_FASTEST = rb_intern("FASTEST");

    rb_const_set(cJPEGReader, rb_intern("DEFAULT_DCT"),
    ID2SYM(j_dct_method_to_id(JDCT_DEFAULT)));
    rb_const_set(cJPEGReader, rb_intern("FASTEST_DCT"),
    ID2SYM(j_dct_method_to_id(JDCT_FASTEST)));
}
