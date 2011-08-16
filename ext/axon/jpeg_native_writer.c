#include "jpeg_common.h"
#include "iccjpeg.h"

static ID id_write, id_io, id_each, id_width, id_height, id_color_model,
	  id_components, id_icc_profile, id_exif, id_quality, id_bufsize,
	  id_image;

struct buf_dest_mgr {
  struct jpeg_destination_mgr pub;

  VALUE io;

  JOCTET *buffer;
  size_t alloc;
};

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
    write_len_i = (size_t)FIX2INT(write_len);
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
    size_t len = dest->alloc - dest->pub.free_in_buffer;
    VALUE str;

    if (len > 0) {
	str = rb_str_new(dest->buffer, len);
	rb_funcall(dest->io, id_write, 1, str);
    }
}

static VALUE
write_scanline(VALUE scan_line, VALUE data2)
{
    JSAMPROW row_pointer[1];
    j_compress_ptr cinfo = (j_compress_ptr)data2;

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
configure(VALUE self, j_compress_ptr cinfo)
{
    VALUE image, width, height, color_model, components, quality;

    quality     = rb_funcall(self, id_quality, 0);

    image       = rb_funcall(self, id_image, 0);
    width       = rb_funcall(image, id_width, 0);
    height      = rb_funcall(image, id_height, 0);
    components  = rb_funcall(image, id_components, 0);
    color_model = rb_funcall(image, id_color_model, 0);

    cinfo->image_width      = FIX2INT(width);
    cinfo->image_height     = FIX2INT(height);
    cinfo->input_components = FIX2INT(components);
    cinfo->in_color_space   = id_to_j_color_space(SYM2ID(color_model));

    jpeg_set_defaults(cinfo);

    if(!NIL_P(quality))
	jpeg_set_quality(cinfo, FIX2INT(quality), TRUE);
}

static void
write_header(VALUE self, j_compress_ptr cinfo)
{
    VALUE icc, exif;

    icc = rb_funcall(self, id_icc_profile, 0);
    if(!NIL_P(icc)) {
	StringValue(icc);
	write_icc_profile(cinfo, RSTRING_PTR(icc), RSTRING_LEN(icc));
    }

    exif = rb_funcall(self, id_exif, 0);
    if (!NIL_P(exif)) {
	StringValue(exif);
	write_exif(cinfo, RSTRING_PTR(exif), RSTRING_LEN(exif));
    }
}

static VALUE
write2(VALUE *args)
{
    VALUE image, self;
    
    self = args[0];
    image = rb_funcall(self, id_image, 0);
    j_compress_ptr cinfo = (j_compress_ptr) args[1];

    configure(self, cinfo);

    jpeg_start_compress(cinfo, TRUE);

    write_header(self, cinfo);
    rb_block_call(image, id_each, 0, 0, write_scanline, (VALUE)cinfo);

    jpeg_finish_compress(cinfo);

    return Qnil;
}

static VALUE
write2_ensure(VALUE *args)
{
    jpeg_destroy_compress((j_compress_ptr) args[1]);
    return Qnil;
}

/*
*  call-seq:
*     writer.write -> nil
*
*  Compress image and write the jpeg.
*/

static VALUE
jpeg_native_write(VALUE self)
{
    struct jpeg_compress_struct cinfo;
    struct buf_dest_mgr mgr;
    VALUE io, rb_bufsize, ensure_args[2];
    int bufsize;

    io         = rb_funcall(self, id_io, 0);
    rb_bufsize = rb_funcall(self, id_bufsize, 0);
    bufsize = FIX2INT(rb_bufsize);

    if (bufsize < 1)
	rb_raise(rb_eRuntimeError, "Buffer size must be greater than zero");

    cinfo.err = &jerr;

    jpeg_create_compress(&cinfo);

    mgr.pub.init_destination = init_destination;
    mgr.pub.empty_output_buffer = empty_output_buffer;
    mgr.pub.term_destination = term_destination;
    mgr.alloc = bufsize;
    mgr.io = io;
    cinfo.dest = (struct jpeg_destination_mgr *)&mgr;

    ensure_args[0] = self;
    ensure_args[1] = (VALUE)&cinfo;

    return rb_ensure(write2, (VALUE)ensure_args, write2_ensure,
		     (VALUE)ensure_args);
}

/* This module provides a single method #jpeg_native_write. It requires that
 * the including class respond to #io, #image, #quality, #exif, #icc_profile,
 * and #bufsize.
 */

void
Init_jpeg_native_writer()
{
    VALUE mAxon = rb_define_module("Axon");
    VALUE mNativeWriter = rb_define_module_under(mAxon, "JPEGNativeWriter");

    rb_define_method(mNativeWriter, "jpeg_native_write", jpeg_native_write, 0);

    id_write = rb_intern("write");
    id_each = rb_intern("each");
    id_io = rb_intern("io");
    id_width = rb_intern("width");
    id_height = rb_intern("height");
    id_color_model = rb_intern("color_model");
    id_components = rb_intern("components");
    id_icc_profile = rb_intern("icc_profile");
    id_exif = rb_intern("exif");
    id_quality = rb_intern("quality");
    id_bufsize = rb_intern("bufsize");
    id_image = rb_intern("image");
}
