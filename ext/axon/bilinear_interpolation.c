#include <ruby.h>

static ID id_image, id_components, id_width_ratio, id_width;


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
            ty       * tx       * c11
 */
static VALUE
interpolate_scanline2(size_t width, double width_ratio, size_t components,
                      double ty, char *scanline1, char *scanline2)
{
    VALUE rb_dest_sl;
    double sample_x, tx, _tx, p00, p10, p01, p11;
    unsigned char *c00, *c10, *c01, *c11, *dest_sl;
    size_t sample_x_i, i, j;

    rb_dest_sl = rb_str_new(0, width * components);
    dest_sl = RSTRING_PTR(rb_dest_sl);
    
    for (i = 0; i < width; i++) {
	sample_x = i / width_ratio;
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

/*
 * call-seq:
 *   image.interpolate_scanline(original_scanlines, q) -> string
 *
 * Expects the instance variable @image to respond to components, and
 * expects the instance variable @width_ratio to respond to the ratio that
 * we will resample our width to.
 */
static VALUE
interpolate_scanline(VALUE self, VALUE orig_scanline1, VALUE orig_scanline2,
                     VALUE rb_sample_y)
{
    VALUE rb_components, rb_width_ratio, rb_image, rb_width;
    double width_ratio, ty, sample_y;
    unsigned char *scanline1, *scanline2;
    size_t width, components;

    rb_width = rb_ivar_get(self, id_width);
    width = FIX2INT(rb_width);

    rb_image = rb_ivar_get(self, id_image);
    rb_components = rb_funcall(rb_image, id_components, 0);
    components = FIX2INT(rb_components);

    rb_width_ratio = rb_ivar_get(self, id_width_ratio);
    width_ratio = NUM2DBL(rb_width_ratio);

    sample_y = NUM2DBL(rb_sample_y);
    ty = sample_y - (int)sample_y;

    scanline1 = RSTRING_PTR(orig_scanline1);
    scanline2 = RSTRING_PTR(orig_scanline2);

    return interpolate_scanline2(width, width_ratio, components, ty, scanline1,
                                 scanline2);
}

void
Init_bilinear_interpolation()
{
    VALUE mAxon = rb_define_module("Axon");
    VALUE mBilinearScaling = rb_define_module_under(mAxon, "BilinearScaling");
    rb_define_method(mBilinearScaling, "interpolate_scanline", interpolate_scanline, 3);
    
    id_width = rb_intern("@width");
    id_width_ratio = rb_intern("@width_ratio");
    id_image = rb_intern("@image");
    id_components = rb_intern("components");
}
