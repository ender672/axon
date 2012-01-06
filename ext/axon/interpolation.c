#include <ruby.h>

/*     c00                 a    c10
 *      --------------------------
 *      |                  |     |
 *      |                  |     |
 *      |                ty|     |
 *      |       tx         |     |
 *      |------------------+-----|
 *      |                 /|     |
 *      |            sample|     |
 *      |                  |     |
 *      |                  |     |
 *      |                  |     |
 *      |                  |     |
 *      --------------------------
 *     c01                 b    c11
 *
 * a      = (1 - tx) * c00 + tx * c10
 * b      = (1 - tx) * c01 + tx * c11
 * sample = (1 - ty) *   a + ty * b
 *
 * sample = (1 - ty) * (1 - tx) * c00 +
 *          (1 - ty) * tx       * c10 +
 *          ty       * (1 - tx) * c01 +
 *          ty       * tx       * c11
 */
static VALUE
bilinear2(size_t width, size_t src_width, size_t components, double ty,
	  char *scanline1, char *scanline2)
{
    VALUE rb_dest_sl;
    double width_ratio_inv, sample_x, tx, _tx, p00, p10, p01, p11;
    unsigned char *c00, *c10, *c01, *c11, *dest_sl;
    size_t sample_x_i, i, j;

    rb_dest_sl = rb_str_new(0, width * components);
    dest_sl = RSTRING_PTR(rb_dest_sl);
    
    width_ratio_inv = (double)src_width / width;

    for (i = 0; i < width; i++) {
	sample_x = i * width_ratio_inv;
	sample_x_i = (int)sample_x;

	tx = sample_x - sample_x_i;
	_tx = 1 - tx;

	p11 =  tx * ty;
	p01 = _tx * ty;
	p10 =  tx - p11;
	p00 = _tx - p01;

	c00 = scanline1 + sample_x_i * components;
	c10 = c00 + components;
	c01 = scanline2 + sample_x_i * components;
	c11 = c01 + components;

	for (j = 0; j < components; j++)
	    *dest_sl++ = p00 * c00[j] + p10 * c10[j] + p01 * c01[j] +
			 p11 * c11[j];
    }
    
    return rb_dest_sl;    
}

/* :nodoc: */

static VALUE
bilinear(VALUE self, VALUE rb_scanline1, VALUE rb_scanline2, VALUE rb_width,
	 VALUE rb_ty, VALUE rb_components)
{
    double ty;
    unsigned char *scanline1, *scanline2;
    int src_line_size;
    size_t width, components, src_width;

    width = NUM2INT(rb_width);
    components = NUM2INT(rb_components);
    ty = NUM2DBL(rb_ty);

    Check_Type(rb_scanline1, T_STRING);
    Check_Type(rb_scanline2, T_STRING);

    src_line_size = RSTRING_LEN(rb_scanline1);

    if (RSTRING_LEN(rb_scanline2) != src_line_size)
	rb_raise(rb_eArgError, "Scanlines don't have the same width.");

    src_width = src_line_size / components - 1;
    scanline1 = RSTRING_PTR(rb_scanline1);
    scanline2 = RSTRING_PTR(rb_scanline2);

    return bilinear2(width, src_width, components, ty, scanline1, scanline2);
}

static VALUE
nearest2(size_t width, size_t src_width, size_t components, char *scanline)
{
    double inv_scale_x;
    VALUE rb_dest_sl;
    char *dest_sl, *xpos;
    size_t i, j;
    
    inv_scale_x = (double)src_width / width;

    rb_dest_sl = rb_str_new(0, width * components);
    dest_sl = RSTRING_PTR(rb_dest_sl);

    for (i = 0; i < width; i++) {
	xpos = scanline + (int)(i * inv_scale_x) * components;
	for (j = 0; j < components; j++)
            *dest_sl++ = *xpos++;
    }

    return rb_dest_sl;    
}

/* :nodoc: */

static VALUE
nearest(VALUE self, VALUE rb_scanline, VALUE rb_width, VALUE rb_components)
{
    unsigned char *scanline;
    size_t width, src_width, src_line_size, components;

    width = NUM2INT(rb_width);
    components = NUM2INT(rb_components);

    Check_Type(rb_scanline, T_STRING);
    src_line_size = RSTRING_LEN(rb_scanline);
    scanline = RSTRING_PTR(rb_scanline);

    src_width = src_line_size / components;
    return nearest2(width, src_width, components, scanline);
}


void
Init_Interpolation()
{
    VALUE mAxon = rb_define_module("Axon");
    /* :nodoc: */
    VALUE mInterpolation = rb_define_module_under(mAxon, "Interpolation");
    rb_define_singleton_method(mInterpolation, "bilinear", bilinear, 5);
    rb_define_singleton_method(mInterpolation, "nearest", nearest, 3);
}
